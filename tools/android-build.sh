#!/usr/bin/env bash
#
# Cross-compiles and links the client as libmain.so for Android arm64, then
# checks that every symbol it still needs is provided by a library it links
# against - so a missing entry point fails here rather than when the phone tries
# to load it.
#
#   ./tools/android-build-deps.sh     # once, builds SDL2 and SDL2_mixer
#   ./tools/android-build.sh
#
# This produces the native library only. It is not an APK: the Java activity,
# the JNI entry point and the Gradle project are still to come, and the UO data
# has to be side-loaded because it cannot be redistributed. See docs/ANDROID.md.
#
set -uo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ndk="${ANDROID_NDK:-/opt/homebrew/share/android-ndk}"
prefix="${ANDROID_SDL2_PREFIX:-$HOME/.cache/orionuo-android/sdl2}"
outdir="${OUT:-$repo/build-android}"
api="${ANDROID_API:-24}"
abi="${ANDROID_ABI:-aarch64-linux-android}"
jobs="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)}"

if [[ ! -d "$ndk" ]]; then
    echo "error: NDK not found at $ndk (brew install --cask android-ndk)" >&2
    exit 2
fi
if [[ ! -f "$prefix/lib/libSDL2.so" ]]; then
    echo "error: no Android SDL2 at $prefix - run tools/android-build-deps.sh first" >&2
    exit 2
fi

host="$(ls "$ndk/toolchains/llvm/prebuilt" | head -1)"
tc="$ndk/toolchains/llvm/prebuilt/$host"
cxx="$tc/bin/${abi}${api}-clang++"
objdir="$outdir/obj"
mkdir -p "$objdir"

# The desktop build's compile database is the source of truth for which files
# are actually part of the client; a bare find would pick up the Windows-only
# ones that CMake never compiles.
db="$repo/build/compile_commands.json"
if [[ ! -f "$db" ]]; then
    echo "error: $db missing - configure the desktop build first:" >&2
    echo "  cmake -G Ninja -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" >&2
    exit 2
fi
sources="$(python3 -c '
import json, sys
db = json.load(open(sys.argv[1]))
print("\n".join(sorted({e["file"] for e in db if e["file"].endswith(".cpp")})))' "$db")"

flags=(
    # -g costs only file size in the .so and makes a crash address on device
    # resolve to a line instead of a guess.
    -std=c++17 -O2 -g -fPIC
    # UO stores tile heights, and a good deal else, in a signed byte that the
    # .mul structs declare as plain char. That is signed on x86, which is where
    # every other build of this client runs, and unsigned on ARM - so a Z of -6
    # read back as 250 and stretched the land quad hundreds of pixels down the
    # screen. Matching the other platforms fixes the whole class at once rather
    # than chasing each struct.
    -fsigned-char
    -DORION_GLES -DORION_CMAKE -DORION_POSIX -DUSE_ORIONDLL=0 -DUSE_WISP=0
    -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DNDEBUG
    -I"$prefix/include" -I"$prefix/include/SDL2" -I"$repo/build"
    -I"$repo/OrionUO/GameObjects" -I"$repo/OrionUO/GLEngine" -I"$repo/OrionUO/GUI"
    -I"$repo/OrionUO/Gumps" -I"$repo/OrionUO/Managers" -I"$repo/OrionUO/Network"
    -I"$repo/OrionUO/ScreenStages" -I"$repo/OrionUO/TextEngine"
    -I"$repo/OrionUO/Utility" -I"$repo/OrionUO/Walker" -I"$repo/OrionUO/Wisp"
    -I"$repo/OrionUO"
    # CMake force-includes stdafx.h through the precompiled header; most files
    # rely on that and include nothing themselves.
    -include "$repo/OrionUO/stdafx.h"
)

echo "building for $abi (api $api) with $jobs jobs"
count=0
pids=()
failed="$outdir/failed.txt"
: > "$failed"

# A .cpp being older than its .o is not enough to skip it: a header it includes
# may have changed, and it silently did - once leaving a library whose callers
# still expected an old constructor signature, which only showed up as an
# unresolved symbol at link time. The compiler records what each object actually
# depended on in a .d file; if any of those is newer, rebuild.
needs_rebuild() {
    local obj="$1" src="$2" dep="$1.d"

    [[ -f "$obj" ]] || return 0
    [[ "$obj" -nt "$src" ]] || return 0
    [[ -f "$dep" ]] || return 0

    local header
    # The .d is "target: a.h b.h \" continuation lines; the paths are what matter.
    for header in $(tr -d '\\' < "$dep" | tr ' ' '\n' | grep -v ':$' | grep -v '^$'); do
        [[ -f "$header" && "$header" -nt "$obj" ]] && return 0
    done

    return 1
}

for src in $sources; do
    rel="${src#$repo/OrionUO/}"
    obj="$objdir/$(echo "$rel" | tr '/' '_' | sed 's/\.cpp$/.o/')"
    if ! needs_rebuild "$obj" "$src"; then
        continue
    fi
    (
        if ! "$cxx" -c "${flags[@]}" -MD -MF "$obj.d" -o "$obj" "$src" 2>"$obj.err"; then
            echo "$rel" >> "$failed"
            echo "  FAILED $rel" >&2
            head -5 "$obj.err" >&2
        fi
    ) &
    pids+=($!)
    count=$((count + 1))
    if (( ${#pids[@]} >= jobs )); then
        wait "${pids[0]}"
        pids=("${pids[@]:1}")
    fi
done
wait

if [[ -s "$failed" ]]; then
    echo "$(wc -l < "$failed" | tr -d ' ') file(s) failed to compile" >&2
    exit 1
fi
echo "compiled $count file(s)"

lib="$outdir/libmain.so"
echo "linking $lib"
if ! "$cxx" -shared -o "$lib" "$objdir"/*.o \
    -L"$prefix/lib" -lSDL2 -lSDL2_mixer \
    -lGLESv1_CM -lEGL -lz -llog -landroid 2>"$outdir/link.err"; then
    echo "link failed:" >&2
    head -30 "$outdir/link.err" >&2
    exit 1
fi

# Everything libmain.so still needs must come from something it links against,
# or it builds here and fails to load on the device.
syslib="$(dirname "$(find "$tc/sysroot/usr/lib/$abi" -name libGLESv1_CM.so | sort | tail -1)")"
provided="$outdir/provided.txt"
: > "$provided"
for dep in "$prefix/lib/libSDL2.so" "$prefix/lib/libSDL2_mixer.so" \
           "$syslib"/lib{GLESv1_CM,EGL,z,log,android,m,dl,c}.so \
           "$tc/sysroot/usr/lib/$abi/libc++_shared.so"; do
    [[ -f "$dep" ]] && "$tc/bin/llvm-nm" -D --defined-only "$dep" 2>/dev/null |
        awk '{print $NF}' | sed 's/@.*//' >> "$provided"
done
sort -u "$provided" -o "$provided"

"$tc/bin/llvm-nm" -u --dynamic "$lib" | awk '{print $NF}' | sed 's/@.*//' | sort -u > "$outdir/needed.txt"
missing="$(comm -23 "$outdir/needed.txt" "$provided")"

echo
ls -lh "$lib"
if [[ -n "$missing" ]]; then
    echo
    echo "unresolved symbols - these would fail to load on device:"
    echo "$missing" | sed 's/^/  /'
    exit 1
fi
echo "all $(wc -l < "$outdir/needed.txt" | tr -d ' ') undefined symbols resolve against the linked libraries"
