#!/usr/bin/env bash
# Pulls printable strings out of HeyPixelMod's obfuscated core.
D=/tmp/hpac
echo "=== files ==="
find "$D" -name '*.class' | wc -l
echo
echo "=== strings mentioning anything detection-shaped ==="
grep -rhoaE '[ -~]{6,}' "$D" \
  | sort -u \
  | grep -iE 'jvmti|retransform|agent|instrument|attach|inject|cheat|hack|anticheat|外挂|检测|封禁|dll|\.so|process|thread|classloader|reflect|screenshot|report|api\.|http|token|debug|attach|virtualmachine|loadlibrary|patch' \
  | head -80
