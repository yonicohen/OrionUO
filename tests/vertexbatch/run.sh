#!/usr/bin/env bash
#
# Builds and runs the CGLVertexBatch equivalence test. Immediate mode is absent
# from OpenGL ES, so the Android port replaces it with the batch; this checks the
# replacement is pixel-for-pixel identical to what it replaced.
#
#   ./tests/vertexbatch/run.sh
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
out="${OUT:-$here/vertexbatch-test}"

# The shim stdafx.h must win over the client's, so -I$here comes first.
clang++ -std=c++14 -O1 -g \
    -I"$here" \
    -I"$repo/OrionUO/GLEngine" \
    $(sdl2-config --cflags) \
    -DGL_SILENCE_DEPRECATION=1 \
    -Wno-deprecated-declarations \
    -o "$out" \
    "$here/main.cpp" \
    "$repo/OrionUO/GLEngine/GLVertexBatch.cpp" \
    $(sdl2-config --libs) \
    -framework OpenGL

exec "$out"
