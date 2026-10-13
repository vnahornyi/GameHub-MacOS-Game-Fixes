#!/bin/bash
# Installs the GTA V Enhanced (Epic) fix for GameHub + gptk-4.0-2.
# Usage: ./install.sh /path/to/GTAVEnhanced [virtual_container_id]
set -euo pipefail

GAME_DIR="${1:?usage: ./install.sh /path/to/GTAVEnhanced [virtual_container_id]}"
CONTAINER_ID="${2:-1}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WINE_ENGINE="$HOME/Library/Application Support/com.gamemac.www/wine-engine"
REG="$WINE_ENGINE/containers/virtual_containers/$CONTAINER_ID/user.reg"

die() { echo "error: $*" >&2; exit 1; }

[ -f "$GAME_DIR/GTA5_Enhanced.exe" ] || die "GTA5_Enhanced.exe not found in $GAME_DIR"
[ -f "$REG" ] || die "registry not found: $REG (check the container id)"
if pgrep -f wineserver >/dev/null; then
  die "Wine is running. Close the game and the Rockstar Launcher first."
fi

cp "$HERE/files/d3d12.dll" "$GAME_DIR/d3d12.dll"
mkdir -p "$GAME_DIR/gptkfix"
cp "$HERE/files/gptkfix/D3D12Core.dll" "$GAME_DIR/gptkfix/D3D12Core.dll"
echo "copied d3d12.dll and gptkfix/D3D12Core.dll to $GAME_DIR"

BACKUP="$REG.bak-gta5fix-$(date +%Y%m%d%H%M%S)"
cp "$REG" "$BACKUP"
echo "registry backup: $BACKUP"

# Drop any existing GTA5_Enhanced.exe DllOverrides section, then append a fresh one.
export SECTION='[Software\\Wine\\AppDefaults\\GTA5_Enhanced.exe\\DllOverrides]'
awk 'BEGIN { s = ENVIRON["SECTION"] }
     index($0, s) == 1 { skip = 1; next }
     skip && /^\[/ { skip = 0 }
     !skip { print }' "$BACKUP" > "$REG"
{
  echo ""
  echo "$SECTION $(date +%s)"
  echo '"amd_ags_x64"=""'
  echo '"d3d12"="native,builtin"'
  echo '"EOSOVH-Win64-Shipping"=""'
} >> "$REG"
echo "registry: GTA5_Enhanced.exe -> d3d12=native,builtin; amd_ags_x64 and EOS overlay disabled"
echo "done"
