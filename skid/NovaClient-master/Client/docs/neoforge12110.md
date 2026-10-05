# Minecraft 1.21.10 NeoForge 适配

这是独立的普通 NeoForge 游戏 profile，与 EaseCation 的启动、配置和版本判断分开。

- 注入器选项：`Minecraft 1.21.10 (NeoForge)`。
- profile ID：`8`，key / 配置目录：`neoforge-1.21.10`。
- EaseCation 仍为 ID `5`、key `easecation`，只接受原来的 1.21.8 环境。
- DLL：`Client/build-vs2026/Release/NovaClientBase.dll`；注入器：`build/Injector.WinUI.exe`。

## 分析来源

Recaf 只读分析的实际安装环境为 NeoForge 21.10.64：

- 原版游戏 JAR：`C:/Users/13366/AppData/Roaming/.minecraft/versions/1.21.10-NeoForge_21.10.64/1.21.10-NeoForge_21.10.64.jar`。
- 实际加载的命名客户端：`C:/Users/13366/AppData/Roaming/.minecraft/libraries/net/neoforged/minecraft-client-patched/21.10.64/minecraft-client-patched-21.10.64.jar`。
- `version.json`：Minecraft 1.21.10、协议 773、Java 21。

初始化核对游戏版本和 NeoForge 类，解析 Mojang 命名接口及 GLFW 窗口；
必需事件规则未全部安装时停止启用模块。

## 实现

`compat/neoforge12110/` 自己管理名称转换、screen、描述符缓存和 Timer 状态。
`compat/mappings/neoforge12110.inc` 包含独立的 profile、类型别名、成员绑定、
队伍/Backtrack 映射和完整钩子选表。公共功能复用现有 C++/JNI 游戏接口、
转头管理、移动修复和 GLFW 回调，不依赖 EaseCation profile 判断。
EaseCation 专用的 Ixeris 跨线程输入和旧 MSVC 运行库兼容仍只作用于 EaseCation。

| 接口 | 1.21.10 实现 |
| --- | --- |
| 窗口 | `Window.handle()` |
| 鼠标反转 | `invertMouseX()` / `invertMouseY()` |
| 击退 | `getMovement()` / `lerpMotion(Vec3)`，公共模块仍按 8000 单位转换 |
| 皮肤 | `PlayerSkin.body().texturePath()` |
| 世界出生点 | `Level.getRespawnData()`，核对维度 |
| 世界投影 | `renderLevel` 有三个矩阵参数，使用第 5/6 槽的实际视图/投影；第 7 槽仅裁剪 |
| 物品攻击力 | 读取 `ItemAttributeModifiers.modifiers()` 及适用装备槽 |

## 验证

```powershell
Client/test/neoforge12110/run.ps1
# 额外客户端 Mixin 的运行时字节码可覆盖审计：
Client/test/neoforge12110/run.ps1 -RuntimeDumpDirectory "$env:TEMP/nova-hook-in"
tools/test-injector-single-file.ps1
```

独立 NeoForge 表：577 个 JNI 绑定、18 个目标类（含代理）、116 项
原生回调描述符/改写/字节码检查全部通过。
Lunar 1.21.11 回归：761 个 JNI 绑定、21 个类、123 项检查全部通过。
EaseCation 钩子已恢复原表；原有 FakePlayer 爆炸模拟的
`ClientboundExplodePacket.radius:F` 审计失败与改动前 HEAD 一致。
离线结果不等同于游戏内验收，本次未自动注入或修改游戏安装文件。
