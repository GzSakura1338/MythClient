# Lunar 1.8.9 适配分析

分析日期：2026-09-17。先使用 Recaf MCP 静态反编译和字节码核对，再在同一 Lunar 构建中完成 Nova 运行时注入测试。未修改 Lunar 类或安装文件。

## 分析基线

- 本次观察到的窗口：Lunar Client 1.8.9 (v2.22.37-2638)。
- JVM：Zulu 17.0.18 x64；已加载 lwjgl64.dll，未观察到 glfw.dll。
- 启动日志：`C:\Users\13366\.lunarclient\profiles\1.8\logs\ichor-boot.log`。
- 日志标识：Config hash `72c13d03`，File hash `f5073303`。
- 日志中的 Lunar commit：`0f1f74fa88d8ba3602a2af212315788c544e8dbe`。
- 对应缓存：`C:\Users\13366\.lunarclient\offline\multiver\cache\72c13d03\f5073303\bake.zip`。
- 缓存 SHA256：`8679C7E2FC6A53DAA8CCC155B495FF014139D15AB20B8E771A61A08895EA040B`。
- Recaf 从该缓存识别了 19955 个类；Minecraft 类文件版本为 61。

启动参数含 DisableAttachMechanism，标准 jcmd 附加失败。这仅证明标准 Attach 路径不可用，不能证明原生加载、JNI、JVMTI 能力或重转换必然成功。缓存也不等同于进程内最终类字节，类加载器可能继续执行转换。

## 加载与转换

Recaf 分析了 genesis-0.1.0-SNAPSHOT-all.jar 中的以下类：

- `com.moonsworth.lunar.genesis.Genesis`：建立 Prelaunch、Game 加载阶段。
- `com.moonsworth.lunar.genesis.ClientGameBootstrap`：选择 MC 版本，创建游戏加载器并设置线程 contextClassLoader，计算缓存路径，启动游戏入口。
- `com.moonsworth.lunar.genesis.CRRRICCRROCOHHOHIICIHORCOORRRH`：URLClassLoader 子类，toString 使用 GenesisClassLoader；findClass 读取缓存或转换结果，再执行 defineClass。缓存路径可能还有后续转换阶段。

Nova 的 GetMinecraftClassLoader 当前通过 Render thread 或 net.minecraft.network.Connection 寻找加载器，不应直接用于此版本。需要基于目标版本的 Minecraft 类及已验证的加载器身份解析，不能只用线程名判断。

## 独立 mapping 的可行性

当前缓存中的游戏类和大量原版成员保留可读名称。第一版可维护明确的 Lunar 1.8.9 符号表，无需以完整 kin 解析器作为前置条件。

| 逻辑符号 | 所属类 | 成员 | 描述符 | 类型 |
|---|---|---|---|---|
| Minecraft.Instance | net/minecraft/client/Minecraft | getMinecraft | ()Lnet/minecraft/client/Minecraft; | 静态方法 |
| Minecraft.Player | net/minecraft/client/Minecraft | thePlayer | Lnet/minecraft/client/entity/EntityPlayerSP; | 实例字段 |
| Minecraft.World | net/minecraft/client/Minecraft | theWorld | Lnet/minecraft/client/multiplayer/WorldClient; | 实例字段 |
| Minecraft.Screen | net/minecraft/client/Minecraft | currentScreen | Lnet/minecraft/client/gui/GuiScreen; | 实例字段 |
| Minecraft.Tick | net/minecraft/client/Minecraft | runTick | ()V | 实例方法 |
| Minecraft.Frame | net/minecraft/client/Minecraft | runGameLoop | ()V | 实例方法 |
| Minecraft.UpdateDisplay | net/minecraft/client/Minecraft | updateDisplay | ()V | 实例方法 |

以上取自该缓存的类元数据。Nova Lunar profile 已通过 JNI 在运行进程中解析并读取 Minecraft 单例、玩家、世界、屏幕和游戏类加载器。后续仍应按客户端构建和能力校验，不能宣称适用于所有 Lunar 更新或所有 1.8.9 客户端。

## Lunar 自带的 bridge 接口

legacy-0.1.0-SNAPSHOT-all-nomappings.jar 中的 Minecraft Mixin：

`com.moonsworth.lunar.legacy.HHCCIRHCCCIIRHCROHIORHIRHHIORH.CRRRICCRROCOHHOHIICIHORCOORRRH.RCIRROCCCIIHCIHCCRHHCCHOHHHCHH`

提供 bridge$getPlayer、bridge$getWorld、bridge$getCurrentScreen、bridge$getWindow、bridge$updateDisplay 等接口。缓存中的 Minecraft 已合并这些方法，bridge$getPlayer 的方法体直接返回 thePlayer。

这些是内部接口，不是承诺稳定的公共 API。返回类型仍包含混淆的 Lunar 接口名称，所以不能仅凭 bridge 方法名构造 JNI 调用。基础状态读取可优先使用已核对的原版成员，确有需要时再适配 bridge 接口。

## 绘制与窗口

Recaf 字节码确认 Minecraft.updateDisplay() 调用 org.lwjgl.opengl.Display.update()。

另外分别反编译确认：

- Display.swapBuffers() 调用 drawable.swapBuffers()。
- WindowsContextImplementation.swapBuffers() 取得当前 GL 上下文的 PeerInfo，调用 nSwapBuffers(peer_handle)。
- nSwapBuffers 的描述符为 `(Ljava/nio/ByteBuffer;)V`，修饰符为 private static native。
- bridge$getWindow() 使用 Display.display_impl 获取窗口实现。

这为 LWJGL 2 后端提供了明确的分析入口。运行测试已验证 nSwapBuffers Hook、Win32 窗口解析、OpenGL 4.6 上的 ImGui 初始化、右 Shift 菜单绘制、LWJGL Mouse JNI 方法解析、DEL 卸载和再次注入。Lunar 使用独立的 LWJGL Mouse 抓取实现，不复用 Forge 的 GLFW 窗口指针和输入模式函数。

## kin 格式

Recaf 定位的读取器：

`com.moonsworth.lunar.HRCHROOHRIHCRCRHRIIROCIRHOIRHH.RRCRRCORICCHOHHIRCHIROOHIIOHCO.RIOOCHICIHRHOHCCCCCHOCCCOHCRHI`

该类继承 Lorenz BinaryMappingsReader。读取逻辑为：

1. 大端 int 魔数 99151942，即 0x05E8F046。
2. byte 版本号，当前必须为 1。
3. int 元数据项数，每项两个 readUTF 字符串；该读取实现读取但不保留这些值。
4. int 顶层类数量。
5. 每个类先读取源名称，再递归读取目标名称、内部类、字段、方法。
6. 字段与方法分别包含数量及源名称、描述符、目标名称三元组。

字符串遵循 Java DataInput.readUTF 的 modified UTF-8 语义。后续解析器应校验长度、数量、递归深度、完整消费和描述符，不应将其当成普通 UTF-8 文本。当前只核对了读取实现，未编写或测试解析器。

## Nova 建议边界

同一 DLL 内选择 VersionProfile，统一提供 Mapping、GameAdapter、HookPlan、RenderBackend 和 Capabilities。mapping 解决符号差异；背包、物品、交互和事件语义的差异由适配实现承担。

目标识别、基础游戏对象读取、窗口绘制、菜单输入、安全卸载、错误 profile 拒绝与再次注入已经通过。后续接入游戏功能时仍需逐项增加 1.8.9 适配，不能将当前 Forge 1.20.1 的模块与 Hook 全量启用在 Lunar 中。

Recaf get_modified_classes 返回空列表。
