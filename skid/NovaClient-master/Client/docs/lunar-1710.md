# Lunar 1.7.10 适配

已接入与 Lunar 1.8.9 相同的模块注册列表及原生运行时能力。没有新增 Java 源码。
这里的“接入”指接口实现和离线校验完成，不代表各模块已通过游戏内验证。

## 已接入

- 注入器 / DLL 共用版本目录：ID 4，`lunar-1.7.10`，独立配置目录。
- 独立 `minecraft/compat/mappings/Lunar1710.inc`：核心 JNI、辅助符号、1.7 专用接口和 Hook 表。
- Minecraft 单例、EntityClientPlayerMP、WorldClient、GuiScreen、GuiChat。
- 复用 LWJGL2 渲染和鼠标捕获后端。
- tick、玩家更新、移动输入、旋转、射线、交互、收发包、持物、相机及渲染事件。
- 战斗、移动、搭桥、背包、箱子、物品使用、Teams / AntiBot、HUD 和 ESP 共用现有模块实现。
- BridgeAssist、BlockIn、Breaker、BedPlates、FreeLook、NameTags、2DESP、玩家列表、HUD 延迟和 Backtrack 已处理 1.7 的接口差异。

## 证据

2026-09-21 使用 Recaf MCP 打开本机 1.7 启动日志指向的
`C:/Users/13366/.lunarclient/offline/multiver/cache/10d9b7f1/aed3ca70/bake.zip`。
工作区共 18949 个类。mapping 中的成员以该构建为基线。

## 运行时差异

- 1.7 本地玩家为 EntityClientPlayerMP，运动发包为 sendMotionUpdates；移动输入位于父类 EntityPlayerSP。
- World.getBlock 使用三个 int；射线命中 blockX/Y/Z、sideHit 均为 int。不能套用 BlockPos / IBlockState / EnumFacing。
- RenderManager.instance 和 renderPosX/Y/Z 是静态字段。
- `IsLunar189()` 保持原语义；可共用的原生逻辑使用 `IsLegacyLunar()`，不同 ABI 使用 `IsLunar1710()` 分支。
- C07 挖掘动作使用整数，C08 使用坐标构造，C06 多一个 stance 高度参数。
- 入站包在 `NetworkManager.processReceivedPackets` 主线程队列分发，不走不存在的 `PacketThreadUtil`。
- Tab 列表按玩家名读取 `GuiPlayerInfo`；床使用区块字节数组和 metadata；1.7 没有盔甲架或深海探索者。
- 对外实体坐标统一为脚部基准，保留 1.7 相机矩阵的原始坐标基准。
- Scaffold 的支点黑名单使用 `Block.blockRegistry.getNameForObject`；1.7 的 Block 没有 `Block{minecraft:...}` 格式的 toString，不能按 1.8 的描述解析，否则所有支点都会被排除。

## 离线验证

- `test/lunar1710/run.ps1`：实际 Lunar 缓存中的 JNI 符号、别名和 16 类 Hook 改写/结构检查。
- `test/network/lunar-incoming/run.ps1`：1.7 主线程直接分发与 1.8 调度路径回归；1.8 改写任务还在隔离 JVM 中验证。
- AutoClicker、AutoBlock、BridgeAssist、BlockIn、HypixelAutoPlay、KeepSprint、KillAura 原生回归测试。
- DLL 构建后的静态 TLS 检查必须通过；注入器按单文件发布。
- 对 1.8 的回归脚本设置 `NOVA_LUNAR189_BAKE` 为实际 1.8 缓存，避免误选最新的 1.7 缓存。

没有自动注入游戏；最终类转换和 UI 显示仍需用户注入验证。
