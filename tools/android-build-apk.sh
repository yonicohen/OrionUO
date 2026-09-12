#!/usr/bin/env bash
#
# Packages the client as an installable APK.
#
#   ./tools/android-build-deps.sh    # once: SDL2 and SDL2_mixer for arm64
#   ./tools/android-build.sh         # libmain.so
#   ./tools/android-build-apk.sh     # this
#
# Uses aapt2/d8/apksigner directly rather than Gradle, so the only Java needed
# is a JDK - no Gradle download, no wrapper, no daemon.
#
# The APK contains the client and SDL, but NOT the UO data: those files are
# copyright, cannot be redistributed, and are ~2.6 GB. They have to be pushed to
# the device separately. See docs/ANDROID.md.
#
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
sdk="${ANDROID_SDK:-$HOME/.cache/orionuo-android/sdk}"
prefix="${ANDROID_SDL2_PREFIX:-$HOME/.cache/orionuo-android/sdl2}"
ndk="${ANDROID_NDK:-/opt/homebrew/share/android-ndk}"
buildtools_version="${ANDROID_BUILD_TOOLS:-34.0.0}"
platform_version="${ANDROID_PLATFORM_VERSION:-android-34}"
abi="arm64-v8a"
out="${OUT:-$repo/build-android/apk}"
lib="$repo/build-android/libmain.so"

bt="$sdk/build-tools/$buildtools_version"
android_jar="$sdk/platforms/$platform_version/android.jar"

for needed in "$bt/aapt2" "$bt/d8" "$bt/apksigner" "$bt/zipalign" "$android_jar"; do
    if [[ ! -e "$needed" ]]; then
        echo "error: missing $needed" >&2
        echo "install with: sdkmanager --sdk_root=$sdk 'platforms;$platform_version' 'build-tools;$buildtools_version'" >&2
        exit 2
    fi
done
if [[ ! -f "$lib" ]]; then
    echo "error: $lib not built - run tools/android-build.sh first" >&2
    exit 2
fi

# A failed compile leaves the previous library in place, and packaging it puts a
# stale build on the phone while reporting a fresh one - which has already cost
# an afternoon once. If any source is newer than the library, it did not build.
newer="$(find "$repo/OrionUO" \( -name '*.cpp' -o -name '*.h' \) -newer "$lib" 2>/dev/null |
    sed -n 1p)"
if [[ -n "$newer" ]]; then
    echo "error: $(basename "$lib") is older than $newer" >&2
    echo "       the last build did not succeed - run tools/android-build.sh and fix it" >&2
    exit 2
fi

# Android's d8 crashes on class files from very new JDKs, so prefer a 17 if one
# is installed. Override with JAVA_HOME if you have a different one.
if [[ -z "${JAVA_HOME:-}" ]]; then
    for candidate in /opt/homebrew/opt/openjdk@17 /opt/homebrew/opt/openjdk@21 \
                     /usr/libexec/java_home /opt/homebrew/opt/openjdk; do
        [[ -x "$candidate/bin/javac" ]] && { JAVA_HOME="$candidate"; break; }
    done
fi
export JAVA_HOME="${JAVA_HOME:-/opt/homebrew/opt/openjdk}"
export PATH="$JAVA_HOME/bin:$PATH"

rm -rf "$out"
mkdir -p "$out"/{classes,res,dex,lib/$abi}

echo "compiling java"
find "$repo/android/java" -name '*.java' > "$out/sources.txt"
# android.jar goes on the classpath, not the bootclasspath: the SDL sources use
# lambdas, and android.jar has no LambdaMetafactory. d8 desugars them afterwards,
# which is what Gradle does too.
javac -nowarn -source 11 -target 11 \
    -classpath "$android_jar" -d "$out/classes" @"$out/sources.txt" 2>&1 |
    grep -v "bootstrap class path\|source value\|target value\|deprecat" || true

if ! find "$out/classes" -name '*.class' | grep -q .; then
    echo "error: javac produced no classes" >&2
    exit 1
fi

echo "dexing"
"$bt/d8" --min-api 21 --lib "$android_jar" --output "$out/dex" \
    $(find "$out/classes" -name '*.class') >/dev/null

echo "compiling resources"
"$bt/aapt2" compile --dir "$repo/android/res" -o "$out/res.zip" >/dev/null

echo "linking apk"
# The manifest carries a placeholder version; a release stamps the tag over it
# so the file on a phone can be told apart from the next one.
# --version-code and --version-name only *inject* a value when the manifest has
# none, and ours carries a placeholder, so without --replace-version every
# release came out as the 1.0 written there.
# A local build has to outrank whatever release is on the device, or Android
# refuses it as a downgrade - which it did silently for a while, leaving a phone
# running a release build while every "fix" was reported as installed.
: "${ANDROID_VERSION_CODE:=900000}"
: "${ANDROID_VERSION_NAME:=dev}"

version_args=()
[[ -n "${ANDROID_VERSION_CODE:-}" ]] && version_args+=(--version-code "$ANDROID_VERSION_CODE")
[[ -n "${ANDROID_VERSION_NAME:-}" ]] && version_args+=(--version-name "$ANDROID_VERSION_NAME")
[[ ${#version_args[@]} -gt 0 ]] && version_args+=(--replace-version)

"$bt/aapt2" link \
    -I "$android_jar" \
    --manifest "$repo/android/AndroidManifest.xml" \
    "${version_args[@]+"${version_args[@]}"}" \
    -o "$out/base.apk" \
    "$out/res.zip" >/dev/null

# The native libraries go in lib/<abi>/. libc++_shared comes from the NDK, and
# is what every one of these was built against.
cp "$lib" "$out/lib/$abi/"
cp "$prefix/lib/libSDL2.so" "$prefix/lib/libSDL2_mixer.so" "$out/lib/$abi/"
host="$(ls "$ndk/toolchains/llvm/prebuilt" | head -1)"
cp "$ndk/toolchains/llvm/prebuilt/$host/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" \
    "$out/lib/$abi/"

echo "packaging"
cd "$out"
cp base.apk unsigned.apk
cp dex/classes.dex .
zip -q -u unsigned.apk classes.dex
zip -q -r unsigned.apk lib
"$bt/zipalign" -f 4 unsigned.apk aligned.apk

# Android will only replace an installed app with one signed by the same key, so
# a release built anywhere other than this machine has to use a keystore that
# outlives the build. Point ANDROID_KEYSTORE at one - CI keeps it in a secret -
# and otherwise fall back to a debug key generated here, which is enough to
# install but means a rebuild elsewhere cannot upgrade over it.
keystore="${ANDROID_KEYSTORE:-$HOME/.cache/orionuo-android/debug.keystore}"
keystore_pass="${ANDROID_KEYSTORE_PASS:-android}"
key_pass="${ANDROID_KEY_PASS:-$keystore_pass}"
key_alias="${ANDROID_KEY_ALIAS:-androiddebugkey}"

if [[ ! -f "$keystore" ]]; then
    if [[ -n "${ANDROID_KEYSTORE:-}" ]]; then
        echo "error: ANDROID_KEYSTORE=$keystore does not exist" >&2
        exit 2
    fi

    mkdir -p "$(dirname "$keystore")"
    keytool -genkeypair -keystore "$keystore" -storepass "$keystore_pass" \
        -keypass "$key_pass" -alias "$key_alias" -keyalg RSA -keysize 2048 \
        -validity 10000 -dname "CN=Android Debug,O=Android,C=US" >/dev/null 2>&1
fi

"$bt/apksigner" sign --ks "$keystore" --ks-pass "pass:$keystore_pass" \
    --key-pass "pass:$key_pass" --ks-key-alias "$key_alias" \
    --out "$out/orionuo.apk" aligned.apk
"$bt/apksigner" verify "$out/orionuo.apk" && echo "signature verified"

rm -f base.apk unsigned.apk aligned.apk
echo
ls -lh "$out/orionuo.apk"
echo
echo "install with:  adb install -r $out/orionuo.apk"
echo "the UO data still has to be pushed separately - see docs/ANDROID.md"
