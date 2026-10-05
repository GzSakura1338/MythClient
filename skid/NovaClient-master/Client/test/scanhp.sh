#!/usr/bin/env bash
# What HeyPixelMod's 933 classes talk about, filtered to detection-shaped names.
D=/tmp/hpall
echo "=== detection / network / reflection vocabulary ==="
grep -rhoaE '[ -~]{5,}' "$D" --include='*.class' \
  | sort -u \
  | grep -iE 'jvmti|retransform|redefine|instrument|attach|inject|cheat|hack|anticheat|外挂|作弊|检测|封禁|ban|LoadLibrary|VirtualAlloc|screenshot|report|token|api\.|https?://|socket|websocket|upload|scan|module|clickgui|aim|aura|reach|velocity' \
  | head -100
