#!/usr/bin/env bash
# What HeyPixelMod hooks, and what its obfuscated core is organised as.
J="D:/MCLDownload/Game/.minecraft/mods/4690287328241054167@2@8.jar"
echo "=== mixin configs ==="
for c in heypixel.mixins.json cit.mixins.json obs_overlay.mixins.json; do
    echo "--- $c"
    unzip -p "$J" "$c" 2>/dev/null | head -60
done
echo
echo "=== com/heypixel subpackages ==="
unzip -l "$J" | awk '{print $4}' | grep '^com/heypixel/' | sed 's#/[^/]*$##' | sort | uniq -c | sort -rn | head -50
echo
echo "=== readable (non I/i) class names under com/heypixel ==="
unzip -l "$J" | awk '{print $4}' | grep '^com/heypixel/.*\.class$' | grep -vE '/[Ii]+\$?[0-9]*\.class$' | head -80
