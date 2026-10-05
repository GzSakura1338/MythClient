# Lunar Client 1.21.11

选择注入器中的 **Lunar Client 1.21.11**，配置键为 `lunar-1.21.11`，稳定 ID 为 `7`。DLL 复用现代 Minecraft 模块、UI、HUD、SMTC 和 GLFW/OpenGL 渲染路径。

## 分析依据

本机 2026-09-27 启动日志确认：Minecraft `1.21.11`，Java 21，配置哈希 `41523644`，文件哈希 `2b715c4b`。

实际文件位于 `%USERPROFILE%/.lunarclient/offline/multiver/`：

- `lunar.jar`：Lunar 客户端，不包含 Minecraft 游戏类。
- `cache/41523644/2b715c4b/bake.zip`：本次启动的游戏类及 Lunar mixin 缓存，已在 Recaf 打开分析；ZIP 条目使用点分隔类名，没有 `.class` 后缀。
- `lunar-platform-mappings-v1_21_11.jar`：映射来源，其中 `lunar/lunar2intermediary_1.21.11.kin` 提供 Mojang 名称到 Fabric intermediary 名称的转换，包含继承成员。

SHA-256：

```text
bake.zip: FBE9DD013A9CCA82D5C7B75D9CAAFF6F4C24D4DF16E19D9C18F06C443645DDA8
mapping jar: 9146BCDA62A4ABF7499BFEAD8BC2FBFB5962345D407188D49567AA025E8880E1
```

缓存路径由 Lunar 更新决定，不能将这个缓存哈希当成永久安装路径。

## 实现边界

`src/minecraft/compat/mappings/Lunar12111.inc` 保存类和成员名称映射。KIN 中 10,254 个类全部保留；成员按项目引用的类和成员名筛选。运行时不读取本机 Lunar 映射 JAR，生成的数据直接编入 DLL。

`compat/lunar12111/` 负责名称/描述符转换、初始化与事件规则适配。JNI 入口 `LoadClassInLoader` 和公共成员解析会按当前 profile 转换；不会把 Lunar 伪装成 EaseCation，也不改变已有 profile ID。

已处理的 ABI 差异：

- Minecraft 主类为 `net.minecraft.class_310`；加载后检查游戏版本及 Lunar Genesis 标记。
- GLFW 窗口和 Minecraft `screen` 字段，区别于 26.3 的 SDL/Gui 接口。
- 相机来自 GameRenderer；视角摇晃从 `ClientAvatarState` 读取。
- 输入记录、旋转/移动修复、鼠标交互、槽位伪装及网络事件接入现有模块系统。
- 攻击减速挂钩位于三参数 `Player.causeExtraKnockback`；相机 setup 的首参数为 Level。
- 击退采用 Vec3 数据；挥手仍使用 InteractionHand/ServerboundSwingPacket。
- Backtrack 使用 short 位移及 PositionMoveRotation；队伍颜色使用 ChatFormatting。
- NativeImage 生命周期使用像素指针检查。

这只适配版本接口，不改变不同 Minecraft 版本的游戏机制。例如新版本不会因此获得 1.8 的剑格挡机制。

## 再生成与校验

在仓库根目录运行：

```powershell
./Client/tools/lunar12111/generate.ps1
./Client/test/lunar12111/run.ps1 -Bake '<本次启动实际使用的 bake.zip>'
# 启动时还会应用 Mixin；磁盘缓存校验不能代替运行时字节码校验。
./Client/test/lunar12111/run.ps1 -RuntimeDumpDirectory "$env:TEMP/nova-hook-in"
```

生成器使用 PowerShell/C# 读取 KIN，不创建 Java 源码或 Java 辅助工具。测试通过原生 C++ 翻译实际映射，对缓存检查类、方法、字段及继承关系，并对所有 16 个事件目标类执行字节码改写、边界校验及原生回调签名比对。`adapted.txt` 仅列出明确走其他版本分支的接口。

运行时规则额外适配 KeyboardInput 的 Mixin 提前返回、Minecraft 使用物品方法尾部的局部变量栈帧，以及 MouseHandler 中 MixinExtras 生成的转向桥接方法。校验运行时 dump 前确认文件来自本次 Lunar 1.21.11 启动，避免混用旧版本导出。

构建产物：

```text
Client/build-vs2026/Release/NovaClientBase.dll
build/Injector.WinUI.exe
```

离线检查和构建不等于游戏内验收。由用户卸载旧实例后自行注入，检查 ClickGUI、HUD、移动、背包、场景切换与 DEL 卸载；注入结果及缺失接口会记录到 `nova-base.log`。
