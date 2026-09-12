#!/usr/bin/env bash
#
# Starts the local shard, with a pipe on its stdin so console commands can be
# sent to it while it runs:
#
#   ./start.sh
#   echo 'C' > control.fifo        # who is online
#   echo 'X#' > control.fifo       # save and exit
#
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

pkill -f SphereSvrX64 2>/dev/null || true
sleep 1

rm -f control.fifo
mkfifo control.fifo

# Something has to hold the write end open, or the server sees EOF and stops
# reading the console after the first command.
nohup sh -c 'while true; do sleep 3600; done > control.fifo' >/dev/null 2>&1 &

nohup sh -c './SphereSvrX64 < control.fifo > server.log 2>&1' >/dev/null 2>&1 &

printf 'starting'
for _ in $(seq 1 60); do
    if grep -q "Startup complete" server.log 2>/dev/null; then
        echo
        grep -a "Startup complete" server.log | tail -1
        echo "listening on 2593 - console: echo 'C' > control.fifo"
        exit 0
    fi
    printf '.'
    sleep 1
done

echo
echo "did not report startup within 60s - see server.log" >&2
exit 1
