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

objs=()
for src in "${sources[@]}"; do
    obj="$out/$(basename "${src%.cpp}").o"
    if "$cxx" -c -std=c++14 -O2 -DORION_GLES \
        -I"$here" -I"$repo/OrionUO/GLEngine" \
        -o "$obj" "$src" 2>"$out/err.txt"; then
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
