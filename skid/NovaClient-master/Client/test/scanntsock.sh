#!/usr/bin/env bash
J="/d/MCLDownload/Game/.minecraft/mods/4690372021023046883@3@0.jar"
echo "=== every class mentioning Socket / NeteaseSocket ==="
unzip -l "$J" | awk '$4 ~ /\.class$/ {print $4}' | grep -iE 'socket|network|connec'
echo
echo "=== all strings in the netease jar mentioning socket/ME/AC ==="
rm -rf /tmp/nt && mkdir -p /tmp/nt && unzip -q -o "$J" -d /tmp/nt
grep -rhoaE '[ -~]{4,}' /tmp/nt/com/netease/mc/mod/network 2>/dev/null | sort -u | head -60
