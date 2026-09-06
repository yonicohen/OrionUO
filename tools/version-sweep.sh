#!/usr/bin/env bash
#
# Search for the Orion version a shard's client check will accept.
#
# The client reports its version in the 0xBF/0xFACE handshake; shards running the
# Sphere "orion exclusive" script compare those four bytes and disconnect a few
# seconds into the world if they do not match. This tries candidate versions one
# at a time and reports which one survives.
#
#   UO_ACCOUNT=name UO_PASSWORD=pass ./tools/version-sweep.sh
#
# The account must already have a character - the server only runs the check on
# character login.

set -uo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
client="${CLIENT:-$repo_root/build/OrionUO/OrionUO}"
server="${UO_SERVER:-uo.jmaul.co.uk,2593}"
log="${LOG:-$repo_root/build/OrionUO/sweep.log}"

# How long to stay connected before calling it a pass. The shard kicks ~5-7s
# after entering the world, so this needs headroom.
settle="${SETTLE:-25}"
# Pause between attempts so we are not hammering the server.
cooldown="${COOLDOWN:-5}"

if [[ -z "${UO_ACCOUNT:-}" || -z "${UO_PASSWORD:-}" ]]; then
    echo "error: set UO_ACCOUNT and UO_PASSWORD (an account that already has a character)" >&2
    exit 2
fi

if [[ ! -x "$client" ]]; then
    echo "error: client not found at $client" >&2
    exit 1
fi

# The client expects -account as hex-encoded bytes (see DecodeArgumentString).
hex() { printf '%s' "$1" | xxd -p | tr -d '\n'; }
account_arg="$(hex "$UO_ACCOUNT"),$(hex "$UO_PASSWORD")"

candidates=()
# Real Orion releases, newest first - most likely to be what "latest" means.
for rev in 37 36 35 34 33 32 31 30; do
    for build in 0 1; do
        candidates+=("1.0.$rev.$build")
    done
done
# The Assistant series, in case the check reads its version instead.
for rev in 37 36 35; do
    candidates+=("3.0.$rev.0")
done
# The value the stock script hardcodes, and a couple of degenerate ones.
candidates+=("1.0.0.0" "0.0.0.0" "1.0.1.0" "2.0.0.0")

echo "sweeping ${#candidates[@]} candidates against $server"
echo "account: $UO_ACCOUNT   settle: ${settle}s   cooldown: ${cooldown}s"
echo

for version in "${candidates[@]}"; do
    pkill -f "[O]rionUO -login" 2>/dev/null
    sleep 1

    : > "$log"
    ( cd "$(dirname "$client")" && nohup "$client" \
        "-login $server" \
        "-orionversion $version" \
        "-account $account_arg" \
        "-autologin 1" \
        "-fastlogin" \
        > "$log" 2>&1 & ) 2>/dev/null

    sleep "$settle"
    pkill -f "[O]rionUO -login" 2>/dev/null
    sleep 2   # let the buffered log flush on exit

    if grep -q "ORION version" "$log" 2>/dev/null; then
        verdict="rejected"
    elif grep -q "Login Complete" "$log" 2>/dev/null; then
        verdict="ACCEPTED - survived ${settle}s in world"
    else
        verdict="inconclusive (never reached the world)"
    fi

    printf '%-12s %s\n' "$version" "$verdict"

    if [[ "$verdict" == ACCEPTED* ]]; then
        cp "$log" "$repo_root/build/OrionUO/sweep-success-$version.log"
        echo
        echo "Accepted version: $version"
        echo "Run the client normally with:  -orionversion $version"
        exit 0
    fi

    sleep "$cooldown"
done

echo
echo "No candidate accepted. The shard's check wants something outside this list;"
echo "the value is in their f_orion_exclusive_sphere_ini IF block."
exit 1
