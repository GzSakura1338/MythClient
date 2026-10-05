#!/usr/bin/env bash
# Identify every mod the Buji client has installed.
M="/d/MCLDownload/Game/.minecraft/mods"
for j in "$M"/*.jar; do
    echo "=================================================================="
    echo "### $(basename "$j")"
    unzip -p "$j" META-INF/mods.toml 2>/dev/null | grep -iE 'modId|displayName|authors|description|version=' | head -8
    unzip -p "$j" fabric.mod.json 2>/dev/null | head -12
    unzip -p "$j" META-INF/MANIFEST.MF 2>/dev/null | grep -iE 'Agent-Class|Premain|Main-Class|Implementation|Automatic' | head -6
    unzip -l "$j" 2>/dev/null | awk '$4 ~ /\.dll$|\.so$|\.exe$/ {print "  PAYLOAD: " $4}'
done
