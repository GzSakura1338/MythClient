#!/usr/bin/env bash
# Which installed mods transform KeyboardInput / MouseHandler / LocalPlayer?
M="/d/MCLDownload/Game/.minecraft/mods"
for j in "$M"/*.jar; do
    n=$(basename "$j")
    hits=$(unzip -p "$j" '*.class' 2>/dev/null | grep -acE 'net/minecraft/client/player/KeyboardInput|net/minecraft/client/MouseHandler')
    if [ "$hits" != "0" ]; then
        echo "=== $n  ($hits class-byte hits)"
        unzip -p "$j" '*.class' 2>/dev/null \
          | grep -aoE 'net/minecraft/client/(player/KeyboardInput|MouseHandler)[A-Za-z]*' | sort | uniq -c
    fi
done
echo
echo "=== mixin configs mentioning input classes ==="
for j in "$M"/*.jar; do
    for c in $(unzip -l "$j" 2>/dev/null | awk '$4 ~ /mixins\.json$/ {print $4}'); do
        echo "--- $(basename "$j") :: $c"
        unzip -p "$j" "$c" 2>/dev/null | grep -iE 'KeyboardInput|MouseHandler|Input|LocalPlayer|GameRenderer' | head -12
    done
done
