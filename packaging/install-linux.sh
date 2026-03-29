#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VST3_DIR="$HOME/.vst3"
PLUGIN_NAME="HALATION.vst3"
VST3_SRC="$SCRIPT_DIR/VST3/$PLUGIN_NAME"

if [ ! -d "$VST3_SRC" ]; then
    echo "Error: $PLUGIN_NAME not found at $VST3_SRC"
    echo "Make sure you extracted the zip and are running this script from inside it."
    exit 1
fi

mkdir -p "$VST3_DIR"
rm -rf "$VST3_DIR/$PLUGIN_NAME"
cp -r "$VST3_SRC" "$VST3_DIR/"
echo "Installed $PLUGIN_NAME to $VST3_DIR/"
echo "Restart your DAW to pick up the new plugin."
