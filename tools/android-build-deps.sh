#!/usr/bin/env bash
#
# Cross-builds the native dependencies the Android client needs: SDL2 and
# SDL2_mixer, for arm64-v8a. Everything else it links against (GLES, EGL, zlib,
# log, android) already ships in the NDK sysroot.
#
#   ./tools/android-build-deps.sh [install-prefix]
#
# Prints the prefix on success. Defaults to ~/.cache/orionuo-android/sdl2, which
# is where tools/android-build.sh and tests/gles/check.sh look.
#
set -euo pipefail

prefix="${1:-$HOME/.cache/orionuo-android/sdl2}"
sdl_version="${SDL2_VERSION:-2.32.10}"
mixer_version="${SDL2_MIXER_VERSION:-2.8.1}"
ndk="${ANDROID_NDK:-/opt/homebrew/share/android-ndk}"
abi="${ANDROID_ABI_CMAKE:-arm64-v8a}"
api="${ANDROID_API:-24}"

if [[ ! -d "$ndk" ]]; then
    echo "error: NDK not found at $ndk (brew install --cask android-ndk)" >&2
    exit 2
fi

if [[ -f "$prefix/lib/libSDL2.so" && -f "$prefix/lib/libSDL2_mixer.so" ]]; then
    echo "$prefix"
    exit 0
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

common=(
    -G Ninja
    -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake"
    -DANDROID_ABI="$abi"
    -DANDROID_PLATFORM="android-$api"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX="$prefix"
)

if [[ ! -f "$prefix/lib/libSDL2.so" ]]; then
    echo "fetching and building SDL2 $sdl_version for $abi" >&2
    curl -sL -o "$work/SDL2.tar.gz" \
        "https://github.com/libsdl-org/SDL/releases/download/release-$sdl_version/SDL2-$sdl_version.tar.gz"
    tar xzf "$work/SDL2.tar.gz" -C "$work"
    cmake -S "$work/SDL2-$sdl_version" -B "$work/sdl-build" "${common[@]}" \
        -DSDL_STATIC=ON -DSDL_SHARED=ON >&2
    ninja -C "$work/sdl-build" >&2
    cmake --install "$work/sdl-build" >&2
fi

if [[ ! -f "$prefix/lib/libSDL2_mixer.so" ]]; then
    echo "fetching and building SDL2_mixer $mixer_version for $abi" >&2
    curl -sL -o "$work/SDL2_mixer.tar.gz" \
        "https://github.com/libsdl-org/SDL_mixer/releases/download/release-$mixer_version/SDL2_mixer-$mixer_version.tar.gz"
    tar xzf "$work/SDL2_mixer.tar.gz" -C "$work"
    # SDL_mixer's finder does not look inside the NDK's find root, so point it at
    # the SDL2 just installed. FluidSynth is not available cross-built, so MIDI
    # goes through the bundled timidity instead.
    cmake -S "$work/SDL2_mixer-$mixer_version" -B "$work/mixer-build" "${common[@]}" \
        -DSDL2_LIBRARY="$prefix/lib/libSDL2.so" \
        -DSDL2_INCLUDE_DIR="$prefix/include/SDL2" \
        -DCMAKE_FIND_ROOT_PATH="$prefix" \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH \
        -DSDL2MIXER_VENDORED=OFF -DSDL2MIXER_SAMPLES=OFF \
        -DSDL2MIXER_FLAC=OFF -DSDL2MIXER_MOD=OFF -DSDL2MIXER_OPUS=OFF \
        -DSDL2MIXER_WAVPACK=OFF -DSDL2MIXER_MP3=ON \
        -DSDL2MIXER_MIDI=ON -DSDL2MIXER_MIDI_FLUIDSYNTH=OFF \
        -DSDL2MIXER_MIDI_TIMIDITY=ON >&2
    ninja -C "$work/mixer-build" >&2
    cmake --install "$work/mixer-build" >&2
fi

echo "$prefix"
