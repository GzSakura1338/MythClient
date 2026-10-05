# 多版本注入结构

Nova 使用同一份 `NovaClientBase.dll` 支持多个客户端。注入器把所选客户端的数值 ID 放进 `NovaBootstrap`，反射加载器将这块参数传给 DLL，DLL 在安装任何 Java 或渲染钩子前选择并验证对应 profile。

## 当前 profile

| Key | 客户端 | 渲染后端 | 当前模块 |
|---|---|---|---|
| `forge-1.20.1` | Minecraft 1.20.1 Forge | GLFW | 公共模块集合，以 `Registry.cpp` 为准 |
| `lunar-1.8.9` | Lunar Client 1.8.9 | LWJGL 2 | 公共模块集合，以 `Registry.cpp` 为准 |
| `minecraft-26.3` | Minecraft 26.3 | SDL3 / OpenGL 或 Vulkan，自动识别 | 公共模块集合；实体、背包、放置、网络、事件和皮肤已接入，游戏内行为待用户验证 |

各版本共用模块注册和界面，保留 AutoBlock 设置。剑格挡仅在 Lunar 1.8.9 执行；1.20.1/26.3 开启 AutoBlock 不执行旧协议、不阻止普通攻击，也不替换为盾牌逻辑。版本不存在的机制允许无效果。

Lunar profile 会在运行中的 JVM 内验证 `net/minecraft/client/Minecraft`、Genesis、LWJGL Mouse，以及独立 mapping 中声明的 Minecraft 单例、玩家、世界和屏幕成员。验证不通过时注入器收到失败状态，DLL 不会安装渲染钩子。

## 文件职责

- `shared/ClientProfiles.inc` 是注入器前端、命令行注入器和 DLL 共用的版本目录。
- `shared/Bootstrap.h` 定义跨进程参数 ABI 和初始化状态。
- `Client/src/minecraft/compat/mappings/*.inc` 保存每个版本独立的 Java 名称与描述符。
- `Client/src/minecraft/compat/mappings/forge1201.inc` 集中保存 Forge 1.20.1 的基础映射、各游戏接口的 JNI 名称与描述符、物品/方块映射表、Hook 重定向表和注册表。调用位置通过 `NOVA_FORGE1201_*` 分区宏包含所需内容；该文件不能添加 `#pragma once`。JNI 句柄及运行逻辑仍由各模块管理。
- `Client/src/minecraft/compat/mappings/Lunar189.inc` 集中声明 Lunar 类、字段和方法；`LunarRuntime.cpp` 实现版本语义转换与 JNI 引用管理。
- `Client/src/minecraft/compat/Profile.cpp` 把目录项关联到 mapping、渲染后端和能力位。
- `Client/src/minecraft/compat/minecraft263/Minecraft263*.cpp` 使用 `minecraft263.inc` 实现版本、后端、GUI、鼠标、实体、物品、容器、网络、皮肤及事件适配；26.3 的详细边界见 [minecraft-26.3-mapping.md](minecraft-26.3-mapping.md)。
- `Client/src/minecraft/host/render/OverlayHost.cpp` 根据 profile 选择 GLFW、LWJGL 2、SDL3/OpenGL 或 `VulkanOverlay.cpp` 的交换链钩子，共用 ClickGUI 与 Win32 输入层。
- `Client/src/minecraft/modules/Registry.cpp` 根据能力位决定可注册的模块集合。

## 增加版本

完整步骤与排错清单见 [新增可注入 Minecraft 版本指南](adding-client-version.md)。当前为源码扩展，需要重编译 DLL、CLI 和 WinUI；不支持放入外部 mapping 后自动加载。下面仅列结构摘要，新增版本还必须接入游戏接口分发、Java bridge、改写规则选择及卸载路径。

1. 在 `shared/ClientProfiles.inc` 追加一个唯一 ID、CLI key 和显示名称。
2. 在 `Client/src/minecraft/compat/mappings` 增加该版本的 mapping 文件。
3. 在 `Client/src/minecraft/compat/Profile.cpp` 指定 mapping、渲染后端和能力位。
4. 如果客户端采用新的窗口或渲染库，在 `RenderBackend` 和 `GlHook.cpp` 增加后端实现。
5. 为需要的游戏功能增加版本适配函数和能力位，通过运行时成员解析后再启用对应模块。
6. 在目标客户端中验证 profile 拒绝错误版本、成功绘制、输入捕获、DEL 卸载和二次注入。

WinUI 会从嵌入的 `ClientProfiles.inc` 自动生成版本下拉框。命令行格式为：

```text
NovaInjector.exe <pid> <absolute-dll-path> <client-key>
```

Forge/Lunar 原生渲染钩子安装后注入器即返回成功；26.3 等待首个 Nova 帧后返回成功。26.3 注入时需保持窗口正常绘制，自动跟随游戏的 OpenGL/Vulkan 后端。使用 `tools/build-injector.cmd` 同时重建原生 CLI 和 WinUI，使它们的版本目录一致。

## Lunar 适配进度（2026-09-17）

以下是分阶段历史记录，当前模块集合以 `Client/src/minecraft/modules/Registry.cpp` 为准，历史测试不代表后续改动均已完成游戏内验收。

目标仍为 Forge 的全部 22 个模块。注册模块不代表已经完成行为验收。

| 模块 | 当前 Lunar 状态 |
|---|---|
| ClickGUI、ArrayList、Watermark | 已注册；此前完成菜单、渲染及卸载测试 |
| AutoClicker、Sprint、FullBright | 已注册，接口已接入；仍需逐项验证点击、冲刺、亮度/夜视及关闭恢复 |
| AimAssist | 已注册，旋转事件已接入；Normal/Silent 对目标玩家的行为仍需验收 |
| KillAura | 已注册，实体、旋转、射线、攻击及挥手接口已接入；目标筛选和攻击行为待验收 |
| Velocity、AntiKb | 已注册，网络、实体和运动接口已接入；延迟击退、重放与跳跃重置待验收 |
| Stuck、BridgeAssist | 等待方块、移动输入及事件适配 |
| FastPlace | 已注册，使用键、方块物品、右键延迟及 Motion 事件已接入；放置行为待验收 |
| Scaffold | 等待完整放置、背包及跳 tick 适配 |
| ChestStealer、InvManager、AutoTools | 容器读取、点击和关闭接口已接入；等待物品属性、工具评分及行为验收 |
| Compass、ChestEsp、NameTags、TargetHud | 实体接口已接入；等待各自投影、箱子、皮肤、计分板或事件适配 |
| Example | 已注册；检查代码确认它仅展示配置并记录日志，不依赖游戏版本或事件 |

本轮构建及运行证据：

- Release DLL 构建通过。
- PID 55112 的旧实例日志确认保存配置并自卸载，随后新实例注入成功。
- 单人世界内日志：`world validation basic=1 lists=1 ray=1 identity=1 players=1 entities=0 ticks=31106`。检查覆盖实体位置/前一 tick 位置、眼睛位置、碰撞箱、血量、ID、类型、朝向、世界列表、射线、碰撞箱截线、按 ID 查找与命中对象创建；附近实体列表为空，不能据此认定非玩家实体行为已验收。
- `ValidateWorld` 在渲染线程首次遇到有效玩家和世界时执行只读检查，每次注入最多一次；不切换模块或修改玩家。
- 字节码重定向现支持显式选择 `GETFIELD (0xB4)`；缺省规则仍仅匹配方法调用。字段 hook 必须保留接收者与返回类型。
- 独立字段测试通过 JVM `-Xverify:all` 并执行，覆盖 float、double、boolean、数组、分支/异常表、ordinal，以及错误签名拒绝后字节不变。
- 现有字节码测试补齐测试桥接的 `packetIn` 声明后，C++ 与 JVM 检查均报告 0 failures；ClientLevel、LivingEntityRenderer 因缺少提取的类样本而跳过，不能算全量 Forge 回归。

Recaf 已确认 `EntityPlayerSP.onUpdateWalkingPlayer()V` 通过字段读取 yaw/pitch/onGround 并构造 C03 系列包；`onLivingUpdate()V` 调用 MovementInput.updatePlayerMoveState。事件表在 `minecraft/compat/mappings/Lunar189.inc` 的 `NOVA_LUNAR189_EDITS` 分区，当前覆盖 tick、玩家 tick、移动输入、移动发包旋转、跳跃、移动朝向、攻击朝向、物品射线、实体/生物视线、第三人称旋转动画及网络收发。移动输入采用调用包装，以保留 Lunar 原方法中的提前返回和自身逻辑。尚未覆盖卡方块、跳 tick 及物品发包事件。

后续在线证据：

- tick=37929937 的一次注入注册 19 个 native hook、7 个模块，7 个类全部重转换及改写成功；游戏内收到 tick、移动输入、motion 回调，世界接口检查再次通过。
- tick=37946046 发起卸载，7 个类均还原，tick=37946640 自卸载成功。此版卸载在还原类之后检查 JVMTI 中尚未退出的旧 Java 方法和 native 桥接帧，然后注销 native 方法，再释放 DLL。
- Runtime 现在提供本地命名事件 `Local\NovaUnload-<pid>`，用于走同一套卸载流程，便于重复测试。该事件必须以正在运行的实例为准；发出信号本身不是卸载成功的证据。
- 在线改写成功仅证明安装、验证器和已观察到的回调路径可用；尚未证明 AimAssist 对另一名玩家的两种模式及第三人称表现一致。

网络适配证据与边界：

- 8 个目标类的全部规则在 Recaf 原始字节码上匹配并通过结构检查。新增接口调用测试通过 JVM `-Xverify:all` 与执行，覆盖五字节 invokeinterface 重定向及调用后的插入；保留的额外字节写为 NOP，分支与异常表长度不变。
- tick=38677125 注入注册 21 个 native hook、10 个模块，8 个类全部改写；客户端入站、出站回调均被观察到。tick=38677750 的 `network values=1 movement/placement reads=1` 验证了构造但不发送的样本包分类、击退数值，以及只读移动/放置接口。
- 出站覆盖 NetworkManager 两个 sendPacket 重载与待发队列；按 direction 过滤客户端连接，避免单人 JVM 的内置服务端触发客户端模块。入站仅向 NetHandlerPlayClient 发布事件，保留原 processPacket 的 Java 异常流程。
- 1.8.9 以 C08 的方向 255 表示仅使用物品；击退延迟对应 S12/S14/S18/S32/S08。现代 Ping 对应旧版交易确认 S32，1.8.9 没有独立 look-at 包，协议本身不存在的类型不伪造。
- 出站重入使用 Win32 TlsAlloc/TlsGetValue，而非反射加载不支持的 MSVC thread_local；DLL 中 gs:[58h] 读取仍为 0。卸载后释放该槽位、待发送标记及网络全局引用。
- 10 模块版卸载时 8 类已还原，tick=38834343 自卸载成功。随后源码加入 KillAura 与 Example 注册，总数为 12；这些模块的玩法验收仍未完成。

背包与容器适配证据与边界：

- Recaf 核对 InventoryPlayer 的 mainInventory/armorInventory 数组、GuiContainer.inventorySlots、Container.windowId/getSlot、ContainerChest.numRows/lowerChestInventory、Slot.getStack、PlayerControllerMP.windowClick 及 EntityPlayerSP.closeScreen。独立映射与原生实现已接入 GameInventory 的槽位、容器、标题、点击、关闭及包接口。
- 1.8.9 空槽返回有效读取结果和 null；无副手槽，读取副手返回 false。物品数组 0..8 对应容器 36..44，9..35 对应原槽号，护甲数组 0..3 对应容器 8..5。Throw 转换为旧版点击模式 4，点击前核对当前打开容器 ID。
- 标题从箱子容器的下层库存 IWorldNameable.getDisplayName 读取，可保留命名箱子及末影箱的实际标题；翻译通过 I18n.format。普通背包标题使用 container.crafting；尚未实现其他容器专有标题，目前标题调用方只筛选箱子。
- tick=38934703 的旧实例卸载已确认；tick=39739859 新实例记录 `inventory slots=1 items=36 armor=4 occupied=16 translation=1 packets=1`。覆盖全部 40 个槽与容器的对象一致性、空槽、负索引和越界拒绝、无副手语义、翻译，以及只构造不发送的关闭/使用包分类。
- 同次注入 8 个目标类全部改写，世界读取检查通过，附近列表 `players=9 entities=3`；这只证明读取结果，不能据此认定目标模块行为已验收。
- Release 构建通过。未自动移动或丢弃物品；点击/关闭的游戏内行为、箱子屏幕标题仍待实际验收。ReadItemFacts、ItemDisplayName、InventoryHasItem 尚未接入 Lunar，背包模块保持未注册，总数仍为 12/22。
- tick=39760156 的卸载日志确认 8 类还原，tick=39760265 自卸载成功。随后 PID 55112 已退出；最后查询到新的 Lunar 1.8.9 PID 34348，尚未向该新进程注入。此版 DLL 的 gs:[58h] 读取计数为 0，diff 空白检查通过。
