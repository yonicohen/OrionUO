#!/usr/bin/env bash
#
# Start the client on the shard named in shard.conf.
#
#   ./play-ignis.sh                 # just play
#   ./play-ignis.sh -fastlogin      # anything extra is passed to the client
#
# Run ./setup.sh first - it is what puts the UO data in place.

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here"

# The client resolves both uo_debug.cfg and its own writable files relative to
# the working directory, not to argv[0]. The cd above is load-bearing.

shard_name="Ignis UO"
shard_host="uo.jmaul.co.uk"
shard_port="2593"
orion_version=""

if [[ -f shard.conf ]]; then
    while IFS='=' read -r key value; do
        key="${key%%[[:space:]]*}"
        case "$key" in
            SHARD_NAME)    shard_name="$value" ;;
            SHARD_HOST)    shard_host="$value" ;;
            SHARD_PORT)    shard_port="$value" ;;
            ORION_VERSION) orion_version="$value" ;;
        esac
    done < <(grep -E '^[[:space:]]*[A-Z_]+=' shard.conf || true)
fi

if [[ ! -f uo_debug.cfg ]]; then
    echo "The UO data has not been set up yet." >&2
    echo >&2
    echo "Run this first:" >&2
    echo "    $here/setup.sh /path/to/your/Ultima\\ Online" >&2
    exit 1
fi

args=("-login $shard_host,$shard_port")
if [[ -n "$orion_version" ]]; then
    args+=("-orionversion $orion_version")
fi

echo "Connecting to $shard_name ($shard_host:$shard_port)"
exec ./OrionUO "${args[@]}" "$@"
