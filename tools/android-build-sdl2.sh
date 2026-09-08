#!/usr/bin/env bash
#
# Cross-builds SDL2 for Android arm64, which tests/gles/check.sh needs before it
# can compile the renderer itself: everything above GLEngine includes stdafx.h,
# which includes SDL.
#
#   ./tools/android-build-sdl2.sh [install-prefix]
#
# Prints the prefix on success; pass it to check.sh as ANDROID_SDL2_PREFIX.
#
set -euo pipefail

prefix="${1:-$HOME/.cache/orionuo-android/sdl2}"
version="${SDL2_VERSION:-2.32.10}"
ndk="${ANDROID_NDK:-/opt/homebrew/share/android-ndk}"
abi="${ANDROID_ABI_CMAKE:-arm64-v8a}"
api="${ANDROID_API:-24}"

if [[ ! -d "$ndk" ]]; then
    echo "error: NDK not found at $ndk (brew install --cask android-ndk)" >&2
    exit 2
fi

if [[ -f "$prefix/lib/libSDL2.so" ]]; then
    echo "$prefix"
    exit 0
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

echo "fetching SDL2 $version" >&2
curl -sL -o "$work/SDL2.tar.gz" \
    "https://github.com/libsdl-org/SDL/releases/download/release-$version/SDL2-$version.tar.gz"
tar xzf "$work/SDL2.tar.gz" -C "$work"

echo "building for $abi (api $api)" >&2
cmake -G Ninja -S "$work/SDL2-$version" -B "$work/build" \
    -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$abi" -DANDROID_PLATFORM="android-$api" \
    -DCMAKE_BUILD_TYPE=Release -DSDL_STATIC=ON -DSDL_SHARED=ON \
    -DCMAKE_INSTALL_PREFIX="$prefix" >&2
ninja -C "$work/build" >&2
cmake --install "$work/build" >&2

echo "$prefix"
