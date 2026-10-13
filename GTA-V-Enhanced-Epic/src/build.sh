#!/bin/bash
# Rebuilds files/d3d12.dll and files/gptkfix/D3D12Core.dll (brew install mingw-w64).
set -euo pipefail
cd "$(dirname "$0")"
x86_64-w64-mingw32-gcc -shared -O2 -o ../files/d3d12.dll d3d12.c -Wl,--kill-at -luuid -lole32
x86_64-w64-mingw32-gcc -shared -O2 -o ../files/gptkfix/D3D12Core.dll D3D12Core.c
