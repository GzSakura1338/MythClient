#!/usr/bin/env bash
# Track down the Netease launcher's filterkey / filterpath component.
echo "=== launcher json/configs mentioning filterpath ==="
grep -rlaE 'filterpath|filterkey' /d/MCLDownload --include='*.json' --include='*.ini' --include='*.cfg' --include='*.config' --include='*.txt' 2>/dev/null | head -20
echo
echo "=== anything named filter* under MCLDownload ==="
find /d/MCLDownload -iname '*filter*' 2>/dev/null | head -30
echo
echo "=== netease AC-ish module names ==="
find /d/MCLDownload -iname '*anticheat*' -o -iname '*anti_cheat*' -o -iname '*acsdk*' -o -iname '*neprotect*' -o -iname '*nesecurity*' 2>/dev/null | head -30
echo
echo "=== mods folder ==="
ls -la "/d/MCLDownload/Game/.minecraft/mods" 2>/dev/null
