#!/usr/bin/env bash
# Lists the top-level package names inside every mod jar in the Netease game dir.
M="D:/MCLDownload/Game/.minecraft/mods"
for f in "$M"/*.jar; do
    echo "=== $(basename "$f")  ($(stat -c%s "$f") bytes)"
    unzip -l "$f" 2>/dev/null | awk '{print $4}' | grep -E '\.(class|json|toml|cfg)$' | head -8
done
