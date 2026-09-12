#!/usr/bin/env bash
#
# Connects the desktop client to the local shard and logs straight in.
#
# -account takes hex, two digits per character. 796F6E69 is "yoni".
set -euo pipefail

CLIENT="${CLIENT:-/Users/yonicohen/dev/orionuo-android2/build/OrionUO/OrionUO}"
DATA="${DATA:-/Users/yonicohen/dev/Ultima Online}"
ACCOUNT_HEX="${ACCOUNT_HEX:-796F6E69}"

cd "$DATA"
exec "$CLIENT" "-login 127.0.0.1,2593" "-account ${ACCOUNT_HEX},${ACCOUNT_HEX}" \
    -autologin -fastlogin "$@"
