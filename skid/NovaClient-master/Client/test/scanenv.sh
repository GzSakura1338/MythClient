#!/usr/bin/env bash
# What the Netease Environment SDK watches for.
D="D:/MCLDownload/ext/jre-v64-220420/jdk17/bin/libenvsdk.dll"
echo "=== JVMTI / instrumentation vocabulary ==="
grep -aoE '[ -~]{5,}' "$D" | sort -u \
  | grep -iE '^(AddCapabilities|GetLoadedClasses|RetransformClasses|RedefineClasses|SetEventCallbacks|SetEventNotificationMode|ClassFileLoadHook|GetEnv|GetPotentialCapabilities|can_retransform|GetClassSignature|GetAllThreads)' \
  | head -30
echo
echo "=== strings mentioning the JVM tool interface at all ==="
grep -aoE '[ -~]{4,}' "$D" | sort -u | grep -iE 'jvmti|instrument|classfileloadhook' | head -20
echo
echo "=== module / process scanning ==="
grep -aoE '[ -~]{5,}' "$D" | sort -u \
  | grep -iE 'EnumProcessModules|CreateToolhelp|Module32|GetModuleFileName|GetModuleHandle|ReadProcessMemory|OpenProcess|NtQuery|LoadLibrary|VirtualQuery|PEB|Ldr' \
  | head -30
echo
echo "=== Minecraft-shaped names ==="
grep -aoE '[ -~]{4,}' "$D" | sort -u \
  | grep -iE 'minecraft|KeyboardInput|GameRenderer|MouseHandler|hitResult|yRot|rotation|net/minecraft|m_1[0-9]{4}_|f_1[0-9]{4}_' \
  | head -30
echo
echo "=== anti-cheat shaped words ==="
grep -aoE '[ -~]{4,}' "$D" | sort -u \
  | grep -iE 'cheat|hack|外挂|作弊|anti|detect|inject|KillAura|AimAssist|aimbot|aura|reach|velocity|nuker' \
  | head -40
