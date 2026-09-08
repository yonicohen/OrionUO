#!/usr/bin/env bash
#
# Cross-compiles the GLES compatibility layer for Android and checks that every
# GL symbol it needs actually exists in the device's GLES 1.x library.
#
# The renderer is written for desktop OpenGL. This is what catches the places
# that silently differ - GLES has no GLdouble, no display lists, and only the
# float spellings of glOrtho and glClearDepth.
#
#   ./tests/gles/check.sh
#
# Needs the NDK: brew install --cask android-ndk
#
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"

ndk="${ANDROID_NDK:-/opt/homebrew/share/android-ndk}"
if [[ ! -d "$ndk" ]]; then
    echo "error: NDK not found at $ndk (set ANDROID_NDK)" >&2
    exit 2
fi

host="$(ls "$ndk/toolchains/llvm/prebuilt" | head -1)"
tc="$ndk/toolchains/llvm/prebuilt/$host"
api="${ANDROID_API:-24}"
abi="${ANDROID_ABI:-aarch64-linux-android}"
cxx="$tc/bin/${abi}${api}-clang++"

if [[ ! -x "$cxx" ]]; then
    echo "error: no compiler at $cxx" >&2
    exit 2
fi

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

echo "NDK  $(sed -n 's/^Pkg.Revision = //p' "$ndk/source.properties")  ->  $abi api$api"
echo

sources=(
    "$repo/OrionUO/GLEngine/GLVertexBatch.cpp"
    "$here/displaylists.cpp"
)

# The renderer itself needs SDL2 built for Android, because everything above
# GLEngine includes stdafx.h, which includes SDL. Build it with
# tools/android-build-sdl2.sh and point ANDROID_SDL2_PREFIX at the result;
# without it the check still covers the compatibility layer on its own.
sdl="${ANDROID_SDL2_PREFIX:-$HOME/.cache/orionuo-android/sdl2}"
client_flags=()
if [[ -f "$sdl/include/SDL2/SDL.h" ]]; then
    sources+=("$repo/OrionUO/GLEngine/GLEngine.cpp")
    client_flags=(
        -std=c++17
        -DORION_CMAKE -DORION_POSIX -DUSE_ORIONDLL=0 -DUSE_WISP=0
        -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DNDEBUG
        -I"$sdl/include" -I"$sdl/include/SDL2"
        -I"$repo/build"
        -I"$repo/OrionUO/GameObjects" -I"$repo/OrionUO/GUI" -I"$repo/OrionUO/Gumps"
        -I"$repo/OrionUO/Managers" -I"$repo/OrionUO/Network"
        -I"$repo/OrionUO/ScreenStages" -I"$repo/OrionUO/TextEngine"
        -I"$repo/OrionUO/Utility" -I"$repo/OrionUO/Walker" -I"$repo/OrionUO/Wisp"
        -I"$repo/OrionUO"
    )
else
    echo "note: no Android SDL2 at $sdl - checking the compat layer only."
    echo "      run tools/android-build-sdl2.sh to include the renderer."
    echo
fi

objs=()
for src in "${sources[@]}"; do
    obj="$out/$(basename "${src%.cpp}").o"
    # The renderer needs the client's own include set; the compat-layer sources
    # build against the shim stdafx.h in this directory instead.
    flags=(-std=c++14 -O2 -DORION_GLES -I"$here" -I"$repo/OrionUO/GLEngine")
    if [[ "$src" == *"/GLEngine/GLEngine.cpp" ]]; then
        flags=(-O2 -DORION_GLES "${client_flags[@]}")
    fi

    if "$cxx" -c "${flags[@]}" -o "$obj" "$src" 2>"$out/err.txt"; then
        printf "  compiled  %s\n" "$(basename "$src")"
        objs+=("$obj")
    else
        printf "  FAILED    %s\n" "$(basename "$src")"
        sed 's/^/      /' "$out/err.txt"
        exit 1
    fi
done

# Every GL entry point the objects still need must exist in the GLES 1.x library
# the device ships, or it links on the host and fails on the phone.
lib="$(find "$tc/sysroot/usr/lib/$abi" -name libGLESv1_CM.so | sort | tail -1)"
echo
echo "checking symbols against $(basename "$(dirname "$lib")")/$(basename "$lib")"

missing=0
for sym in $("$tc/bin/llvm-nm" -u "${objs[@]}" | grep -oE '\bgl[A-Za-z0-9]+' | sort -u); do
    if "$tc/bin/llvm-nm" -D "$lib" 2>/dev/null | grep -qw "$sym"; then
        printf "  %-24s present\n" "$sym"
    else
        printf "  %-24s MISSING FROM GLES\n" "$sym"
        missing=$((missing + 1))
    fi
done

echo
if (( missing > 0 )); then
    echo "$missing symbol(s) are not in GLES 1.x - these would fail on device"
    exit 1
fi
echo "all GL symbols resolve against Android GLES 1.x"
