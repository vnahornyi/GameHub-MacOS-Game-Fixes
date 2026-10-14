#!/bin/bash
# Rebuilds files/d3d12.dll (brew install mingw-w64).
set -euo pipefail
cd "$(dirname "$0")"
x86_64-w64-mingw32-gcc -shared -O2 -s -o ../files/d3d12.dll d3d12.c d3d12.def -ldxguid
