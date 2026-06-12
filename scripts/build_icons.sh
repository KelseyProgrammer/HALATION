#!/usr/bin/env bash
# build_icons.sh
# Converts packaging/icon.svg → packaging/halation.icns
# Converts packaging/background.svg → packaging/resources/background.png
#
# Requirements (macOS):
#   brew install librsvg
#
# Run from repo root:
#   bash scripts/build_icons.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PACKAGING="$REPO_ROOT/packaging"
ICON_SVG="$PACKAGING/icon.svg"
BG_SVG="$PACKAGING/background.svg"
ICNS_OUT="$PACKAGING/halation.icns"
BG_OUT="$PACKAGING/resources/background.png"

# ── Prerequisites ──────────────────────────────────────────────────────────────
if ! command -v rsvg-convert &>/dev/null; then
  echo "ERROR: rsvg-convert not found. Install with: brew install librsvg"
  exit 1
fi

if ! command -v iconutil &>/dev/null; then
  echo "ERROR: iconutil not found (macOS only). Run this script on macOS."
  exit 1
fi

echo "→ Building halation.icns from icon.svg"

# ── Build iconset ──────────────────────────────────────────────────────────────
ICONSET=$(mktemp -d)/halation.iconset
mkdir -p "$ICONSET"

# macOS iconset required sizes
#   Each @2x entry is just the next size up, named differently.
# Each entry: "SIZE NAME1 [NAME2]"
# Sizes where a single rasterization maps to two iconset filenames share both.
ICON_ENTRIES=(
  "16   icon_16x16.png"
  "32   icon_16x16@2x.png icon_32x32.png"
  "64   icon_32x32@2x.png"
  "128  icon_128x128.png"
  "256  icon_128x128@2x.png icon_256x256.png"
  "512  icon_256x256@2x.png icon_512x512.png"
  "1024 icon_512x512@2x.png"
)

for entry in "${ICON_ENTRIES[@]}"; do
  sz=$(echo "$entry" | awk '{print $1}')
  name1=$(echo "$entry" | awk '{print $2}')
  name2=$(echo "$entry" | awk '{print $3}')

  echo "  rasterizing ${sz}×${sz}"
  TMP_PNG="$ICONSET/_tmp_${sz}.png"
  rsvg-convert -w "$sz" -h "$sz" "$ICON_SVG" -o "$TMP_PNG"

  cp "$TMP_PNG" "$ICONSET/$name1"
  echo "    → $name1"
  if [ -n "$name2" ]; then
    cp "$TMP_PNG" "$ICONSET/$name2"
    echo "    → $name2"
  fi
  rm "$TMP_PNG"
done

# ── Compile .icns ──────────────────────────────────────────────────────────────
iconutil -c icns "$ICONSET" -o "$ICNS_OUT"
rm -rf "$(dirname "$ICONSET")"
echo "✓ Created $ICNS_OUT"

# ── Installer background ───────────────────────────────────────────────────────
echo "→ Building background.png from background.svg"
rsvg-convert -w 1024 -h 640 "$BG_SVG" -o "$BG_OUT"
echo "✓ Created $BG_OUT"

echo ""
echo "Done. Commit these two files:"
echo "  packaging/halation.icns"
echo "  packaging/resources/background.png"
