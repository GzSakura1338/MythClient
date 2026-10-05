#!/usr/bin/env bash
# Surveys HeyPixel's client-side anti-cheat jar.
J="D:/MCLDownload/Game/.minecraft/mods/4690287328241054167@2@8.jar"
echo "=== top-level entries ==="
unzip -l "$J" | awk '{print $4}' | grep -v '^$' | sed 's#/[^/]*$#/#' | sort -u | head -60
echo
echo "=== non-class files ==="
unzip -l "$J" | awk '{print $4}' | grep -v '\.class$' | grep -v '^$' | head -60
echo
echo "=== package roots under com/ ==="
unzip -l "$J" | awk '{print $4}' | grep '^com/' | cut -d/ -f1-2 | sort -u | head -30
echo
echo "=== manifest ==="
unzip -p "$J" META-INF/MANIFEST.MF 2>/dev/null | head -30
echo "=== mods.toml ==="
unzip -p "$J" META-INF/mods.toml 2>/dev/null | head -40
