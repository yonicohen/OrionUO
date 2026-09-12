#!/usr/bin/env bash
#
# One-time setup: point this package at an Ultima Online installation and play.
#
#   ./setup.sh                                # asks where UO is
#   ./setup.sh "/path/to/Ultima Online"       # or tell it
#   ./setup.sh --no-launch "/path/to/UO"      # set up but do not start
#
# UO's data files are copyright and are not redistributed with this client.
# You need them from an existing install - your shard's package, or the free
# Classic Client from https://uo.com/client-download/ . This script does not
# copy 2.6 GB around: on macOS and Linux it symlinks the data into ./data and
# leaves your install untouched.

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here"

data_dir="$here/data"
launch=1
uo_path=""

for arg in "$@"; do
    case "$arg" in
        --no-launch) launch=0 ;;
        -h|--help)
            sed -n '2,14p' "${BASH_SOURCE[0]}" | sed 's/^#\{0,1\} \{0,1\}//'
            exit 0
            ;;
        *) uo_path="$arg" ;;
    esac
done

say()  { printf '%s\n' "$*"; }
step() { printf '\n==> %s\n' "$*"; }
die()  { printf 'error: %s\n' "$*" >&2; exit 1; }

# --------------------------------------------------------------------------
# 1. Where is Ultima Online?
# --------------------------------------------------------------------------
if [[ -z "$uo_path" ]]; then
    say "This client needs the data files from an Ultima Online installation."
    say "They are copyright and cannot be shipped with it."
    say
    say "That is the folder holding art.mul / ArtLegacyMUL.uop and map0.mul -"
    say "either your shard's download, or an install of the free Classic"
    say "Client from https://uo.com/client-download/ ."
    say
    printf 'Path to your Ultima Online folder: '
    read -r uo_path
    # A path dragged into a terminal arrives quoted and/or backslash-escaped.
    uo_path="${uo_path%\"}"; uo_path="${uo_path#\"}"
    uo_path="${uo_path%\'}"; uo_path="${uo_path#\'}"
    uo_path="${uo_path//\\ / }"
fi

[[ -n "$uo_path" ]] || die "no path given"
[[ -d "$uo_path" ]] || die "'$uo_path' is not a directory"
uo_path="$(cd "$uo_path" && pwd)"

# --------------------------------------------------------------------------
# 2. Does it look like a real install?
# --------------------------------------------------------------------------
# Case-insensitive, because a Linux filesystem will not do it for us.
find_entry() {
    local want="$1" name
    for name in "$uo_path"/*; do
        [[ -e "$name" ]] || continue
        if [[ "$(printf '%s' "${name##*/}" | tr '[:upper:]' '[:lower:]')" == "$want" ]]; then
            printf '%s' "${name##*/}"
            return 0
        fi
    done
    return 1
}

step "Checking $uo_path"

have_art=0 have_map=0
find_entry art.mul            >/dev/null && have_art=1
find_entry artlegacymul.uop   >/dev/null && have_art=1
find_entry map0.mul           >/dev/null && have_map=1
find_entry map0legacymul.uop  >/dev/null && have_map=1

if (( ! have_art || ! have_map )); then
    say "    This does not look like a UO client directory."
    (( have_art )) || say "    missing: art.mul or ArtLegacyMUL.uop"
    (( have_map )) || say "    missing: map0.mul or map0LegacyMUL.uop"
    say
    say "    Point this at the folder that holds the game data itself, not at"
    say "    the installer, the .zip, or the folder containing it."
    die "no UO data found in '$uo_path'"
fi
say "    art and map data: found"

client_cuo="$(find_entry client.cuo || true)"
if [[ -n "$client_cuo" ]]; then
    say "    Client.cuo: found"
else
    say "    Client.cuo: not present - one will be generated"
fi

# --------------------------------------------------------------------------
# 3. Runtime libraries
# --------------------------------------------------------------------------
step "Checking runtime libraries"

os="$(uname -s)"
if [[ "$os" == "Darwin" ]]; then
    # Ask the binary what it actually links rather than trusting a list that
    # rots. Homebrew paths look like /opt/homebrew/opt/<formula>/lib/... on
    # Apple Silicon and /usr/local/opt/<formula>/... on Intel.
    # Done with bash's own string operators rather than sed or awk. The BSD
    # versions macOS ships do not take GNU's \| alternation or \t in a regex,
    # and both failures are silent - a check that quietly matches nothing and
    # reports success is worse than no check.
    missing=()
    while IFS= read -r lib; do
        [[ -n "$lib" ]] || continue

        rest="${lib#/opt/homebrew/opt/}"                       # Apple Silicon
        [[ "$rest" != "$lib" ]] || rest="${lib#/usr/local/opt/}"  # Intel
        [[ "$rest" != "$lib" ]] || continue   # a system library, not Homebrew's
        formula="${rest%%/*}"

        # `brew install sdl2` is what lands sdl2-compat; brew knows the alias,
        # a human reading the error message may not.
        if [[ "$formula" == "sdl2-compat" ]]; then
            formula="sdl2"
        fi
        [[ -e "$lib" ]] || missing+=("$formula")
    done < <(otool -L ./OrionUO 2>/dev/null |
             sed -n 's#^[[:space:]]\{1,\}\(/[^ ]*\).*#\1#p')

    if (( ${#missing[@]} )); then
        # Deduplicate; one formula can supply several dylibs. mapfile would be
        # the obvious tool and does not exist in the bash 3.2 macOS ships.
        missing=($(printf '%s\n' "${missing[@]}" | sort -u))
        if command -v brew >/dev/null 2>&1; then
            say "    Missing Homebrew libraries. Run:"
        else
            say "    Homebrew is not installed. Install it from https://brew.sh"
            say "    then run:"
        fi
        say
        say "        brew install ${missing[*]}"
        say
        die "install the libraries above, then run this script again"
    fi
    say "    all Homebrew libraries present"
elif [[ "$os" == "Linux" ]]; then
    if command -v ldd >/dev/null 2>&1; then
        missing="$(ldd ./OrionUO 2>/dev/null | awk '/not found/ {print $1}' || true)"
        if [[ -n "$missing" ]]; then
            say "    Missing shared libraries:"
            printf '        %s\n' $missing
            say
            say "    On Debian/Ubuntu:"
            say "        sudo apt-get install libsdl2-2.0-0 libsdl2-image-2.0-0 \\"
            say "            libsdl2-mixer-2.0-0 libfreeimage3 libglew2.2 libglu1-mesa"
            say "    On Fedora:"
            say "        sudo dnf install SDL2 SDL2_image SDL2_mixer freeimage glew mesa-libGLU"
            say
            die "install the libraries above, then run this script again"
        fi
        say "    all shared libraries resolve"
    else
        say "    ldd not available - skipping the library check"
    fi
fi

# --------------------------------------------------------------------------
# 4. Link the data into ./data
# --------------------------------------------------------------------------
# The client reads UO data from one directory and writes its own settings into
# that same directory. Linking into ./data rather than pointing the client
# straight at the install keeps those writes out of your UO folder.
step "Linking UO data into $data_dir"

rm -rf "$data_dir"
mkdir -p "$data_dir"

# Never read by this client. Between them these are most of the install.
skip_entry() {
    local lower
    lower="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"
    case "$lower" in
        models|"orion launcher"|oa) return 0 ;;   # Windows launcher and plugins
        *.bik) return 0 ;;                        # intro videos
        *.exe|*.dll|*.pdb) return 0 ;;            # Windows client leftovers
        desktop.ini|thumbs.db|.ds_store) return 0 ;;
        # Orion's own settings, not UO data. The client rewrites these in place,
        # so a symlink would push this package's settings - and any saved
        # account - back into your UO folder. Start from defaults instead.
        orion_options.cfg|macros_debug.cuo|uo_debug.cfg) return 0 ;;
        options_debug.cuo|skills_debug.cuo|gumps_debug.cuo) return 0 ;;
    esac
    return 1
}

linked=0 skipped=0 fallback_copy=0
for src in "$uo_path"/*; do
    [[ -e "$src" ]] || continue
    name="${src##*/}"

    if skip_entry "$name"; then
        skipped=$((skipped + 1))
        continue
    fi

    dst="$data_dir/$name"

    if ln -s "$src" "$dst" 2>/dev/null; then
        linked=$((linked + 1))
    else
        cp -R "$src" "$dst"
        fallback_copy=$((fallback_copy + 1))
    fi

    # A case-sensitive filesystem will not find "tiledata.mul" behind
    # "Tiledata.mul", and the client asks for both spellings depending on the
    # file. An extra lowercase alias costs nothing and fixes it on Linux; on a
    # case-insensitive filesystem the -e test below is already true, so
    # nothing happens.
    lower_name="$(printf '%s' "$name" | tr '[:upper:]' '[:lower:]')"
    if [[ "$lower_name" != "$name" && ! -e "$data_dir/$lower_name" ]]; then
        ln -s "$src" "$data_dir/$lower_name" 2>/dev/null || true
    fi
done

say "    $linked linked, $skipped skipped as unused or not UO data"
(( fallback_copy == 0 )) || say "    $fallback_copy could not be linked and were copied"
say
say "    Not linked, because this client never reads them: Models/,"
say "    'Orion Launcher'/, the .bik intro videos and the Windows .exe/.dll."
say "    Music/ is linked but is only needed for in-game music."

# --------------------------------------------------------------------------
# 5. Client.cuo
# --------------------------------------------------------------------------
# Client.cuo tells the client which UO version and login encryption to use. It
# normally comes from the Windows-only Orion Launcher, so a stock uo.com
# install has none and we write one for 7.0.20.0.
if [[ -z "$client_cuo" ]]; then
    step "Generating Client.cuo (client 7.0.20.0)"
    if command -v python3 >/dev/null 2>&1 && [[ -f make_client_cuo.py ]]; then
        python3 make_client_cuo.py -o "$data_dir/Client.cuo" \
            --client-version CV_70180 --encryption ET_TFISH --version-text 7.0.20.0
        say "    wrote $data_dir/Client.cuo"
    else
        say "    python3 or make_client_cuo.py is missing, so it could not be"
        say "    generated. If your shard ships a Client.cuo, copy it into"
        say "    $data_dir and it will be used instead."
        die "Client.cuo is required and could not be produced"
    fi
fi

# --------------------------------------------------------------------------
# 6. Tell the client where the data is
# --------------------------------------------------------------------------
step "Writing uo_debug.cfg"
printf 'CustomPath=%s\n' "$data_dir" > "$here/uo_debug.cfg"
say "    CustomPath=$data_dir"

if [[ ! -f "$here/uo_4mb_2.sf2" ]]; then
    say
    say "note: uo_4mb_2.sf2 is not here, so MIDI music will be silent."
    say "      MP3 music and sound effects are unaffected."
fi

# --------------------------------------------------------------------------
# 7. Play
# --------------------------------------------------------------------------
shard_host="$(sed -n 's/^SHARD_HOST=//p' shard.conf 2>/dev/null | head -1)"
shard_port="$(sed -n 's/^SHARD_PORT=//p' shard.conf 2>/dev/null | head -1)"
: "${shard_host:=uo.jmaul.co.uk}"
: "${shard_port:=2593}"

# --------------------------------------------------------------------------
# 7a. A double-clickable app, on macOS
# --------------------------------------------------------------------------
# The client is a plain Unix executable, so Finder gives it the generic
# "executable" icon and the Dock shows the raw process name. An icon and a
# proper name need a bundle, which is only a directory with a plist in it - so
# build one here that runs the launcher.
if [[ "$(uname -s)" == "Darwin" && -f "$here/OrionUO.icns" ]]; then
    step "Making Ignis UO.app"
    app="$here/Ignis UO.app"
    rm -rf "$app"
    mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"
    cp "$here/OrionUO.icns" "$app/Contents/Resources/OrionUO.icns"

    cat > "$app/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key><string>Ignis UO</string>
    <key>CFBundleDisplayName</key><string>Ignis UO</string>
    <key>CFBundleIdentifier</key><string>uk.co.jmaul.orionuo</string>
    <key>CFBundleExecutable</key><string>Ignis UO</string>
    <key>CFBundleIconFile</key><string>OrionUO.icns</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>1.0.37.0</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PLIST

    cat > "$app/Contents/MacOS/Ignis UO" <<LAUNCH
#!/bin/bash
# The bundle is inside the package, so the client still runs from the package
# directory, where its data/ and uo_debug.cfg are.
cd "\$(dirname "\${BASH_SOURCE[0]}")/../../.." || exit 1
exec ./play-ignis.sh "\$@"
LAUNCH
    chmod +x "$app/Contents/MacOS/Ignis UO"

    # Finder caches icons per path; touching the bundle makes it re-read.
    touch "$app"
    say "    $app"
    say "    Drag it to the Dock or Applications if you want it there."
fi

step "Ready"
say "    Double-click: Ignis UO.app" 
say "    Play with:   $here/play-ignis.sh"
say "    Equivalent:  cd $here && ./OrionUO \"-login $shard_host,$shard_port\""
say

if (( launch )); then
    say "Starting the client..."
    exec "$here/play-ignis.sh"
fi
