#!/usr/bin/env bash
# What the client logs when a module is on, on Buji.
L="/d/MCLDownload/Game/.minecraft/logs"
echo "########## latest.log: detection-shaped lines ##########"
grep -aiE 'heypixel|observer|anticheat|anti-cheat|检测|黑屋|封禁|screenshot|截图|exception|Nova|Transformer|retransform' "$L/latest.log" \
  | grep -viE '^\s*$' | sed 's/\(.\{200\}\).*/\1.../' | tail -60
echo
echo "########## latest.log: connection / server ##########"
grep -aiE 'Connecting to|Connected to|server joined|logged in|Set UUID|netease|channel' "$L/latest.log" \
  | sed 's/\(.\{180\}\).*/\1.../' | tail -25
