#!/usr/bin/env bash
#
# Build OrionUO on macOS and point it at a UO installation.
#
#   ./tools/setup-macos.sh /path/to/your/UltimaOnline
#
# Re-running is safe; it reuses the existing build directory.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-$repo_root/build}"
build_type="${BUILD_TYPE:-Release}"

uo_path="${1:-}"
if [[ -z "$uo_path" ]]; then
    echo "usage: $0 /path/to/UltimaOnline" >&2
    echo >&2
    echo "The UO directory is the one holding art.mul / artidx.mul (older" >&2
    echo "installs) or the .uop files (7.0.24 and newer)." >&2
    exit 2
fi

if [[ ! -d "$uo_path" ]]; then
    echo "error: '$uo_path' is not a directory" >&2
    exit 1
fi
uo_path="$(cd "$uo_path" && pwd)"

# --- dependencies -----------------------------------------------------------
if ! command -v brew >/dev/null 2>&1; then
    echo "error: Homebrew is required (https://brew.sh)" >&2
    exit 1
fi

missing=()
for formula in cmake ninja sdl2 sdl2_mixer glew; do
    brew list --formula "$formula" >/dev/null 2>&1 || missing+=("$formula")
done

if (( ${#missing[@]} )); then
    echo "==> Installing: ${missing[*]}"
    brew install "${missing[@]}"
fi

# --- build ------------------------------------------------------------------
echo "==> Configuring ($build_type)"
cmake -G Ninja -S "$repo_root" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type"

echo "==> Building"
ninja -C "$build_dir" OrionUO

run_dir="$build_dir/OrionUO"

# --- runtime configuration --------------------------------------------------
# Client.cuo is normally produced by the Windows-only Orion Launcher.
if [[ ! -f "$run_dir/Client.cuo" ]]; then
    echo "==> Generating Client.cuo"
    python3 "$repo_root/tools/make_client_cuo.py" -o "$run_dir/Client.cuo"
else
    echo "==> Keeping existing Client.cuo"
fi

echo "==> Writing uo_debug.cfg -> $uo_path"
printf 'CustomPath=%s\n' "$uo_path" > "$run_dir/uo_debug.cfg"

# --- report what the data directory actually has ----------------------------
echo
echo "==> UO data check in $uo_path"
found_any=0
for f in art.mul artidx.mul ArtLegacyMUL.uop MainMisc.uop map0.mul map0LegacyMUL.uop \
         gumpart.mul gumpidx.mul GumpartLegacyMUL.uop tiledata.mul hues.mul; do
    if [[ -e "$uo_path/$f" ]]; then
        printf '     found   %s\n' "$f"
        found_any=1
    fi
done
if (( ! found_any )); then
    echo "     WARNING: none of the expected UO data files were found here."
    echo "     Check that this is the client directory, not the installer."
fi

# MIDI music needs a soundfont; MP3 music (client >= 3.0.6e) does not.
if [[ ! -f "$run_dir/uo_4mb_2.sf2" ]]; then
    echo
    echo "note: uo_4mb_2.sf2 not present - MIDI music will be silent."
    echo "      MP3 music is unaffected. Drop the soundfont in $run_dir to enable it."
fi

echo
echo "Done. Run it with:"
echo "    cd $run_dir && ./OrionUO"
