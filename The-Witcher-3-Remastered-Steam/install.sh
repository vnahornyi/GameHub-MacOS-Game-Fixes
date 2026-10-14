#!/bin/bash
# Installs The Witcher 3 Remastered (Steam) fix for GameHub + gptk-4.0-2.
# Usage: ./install.sh "/path/to/The Witcher 3" [virtual_container_id]
set -euo pipefail

GAME_DIR="${1:?usage: ./install.sh \"/path/to/The Witcher 3\" [virtual_container_id]}"
CONTAINER_ID="${2:-1}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WINE_ENGINE="$HOME/Library/Application Support/com.gamemac.www/wine-engine"
REG="$WINE_ENGINE/containers/virtual_containers/$CONTAINER_ID/user.reg"
BIN="$GAME_DIR/bin/x64_dx12"
STAMP="$(date +%Y%m%d%H%M%S)"

die() { echo "error: $*" >&2; exit 1; }

[ -f "$BIN/witcher3.exe" ] || die "bin/x64_dx12/witcher3.exe not found in $GAME_DIR"
[ -f "$REG" ] || die "registry not found: $REG (check the container id)"
if pgrep -f wineserver >/dev/null; then
  die "Wine is running. Close the game and Steam in GameHub first."
fi

if [ -f "$BIN/d3d12.dll" ] && ! cmp -s "$HERE/files/d3d12.dll" "$BIN/d3d12.dll"; then
  cp "$BIN/d3d12.dll" "$BIN/d3d12.dll.bak-w3fix-$STAMP"
  echo "existing d3d12.dll backup: $BIN/d3d12.dll.bak-w3fix-$STAMP"
fi
cp "$HERE/files/d3d12.dll" "$BIN/d3d12.dll"
echo "copied d3d12.dll to $BIN"

BACKUP="$REG.bak-w3fix-$STAMP"
cp "$REG" "$BACKUP"
echo "registry backup: $BACKUP"

# Drop any existing witcher3.exe DllOverrides section and trailing blank lines, then append a
# fresh section, so a second run gives the same file.
export SECTION='[Software\\Wine\\AppDefaults\\witcher3.exe\\DllOverrides]'
awk 'BEGIN { s = ENVIRON["SECTION"] }
     index($0, s) == 1 { skip = 1; next }
     skip && /^\[/ { skip = 0 }
     skip { next }
     /^$/ { blank++; next }
     { while (blank > 0) { print ""; blank-- } print }' "$BACKUP" > "$REG"
{
  echo ""
  echo "$SECTION $(date +%s)"
  echo '"d3d12"="native,builtin"'
} >> "$REG"
echo "registry: witcher3.exe -> d3d12=native,builtin"
echo "done"
