# 新增可注入 Minecraft 版本指南

本文面向希望为 Nova 增加客户端版本的开发者，按当前源码（2026-09-18）说明接入流程。文中路径除链接外均相对仓库根目录。

Nova 的目标是：注入器选择客户端，同一个 `NovaClientBase.dll` 根据 profile 使用对应的 mapping、游戏接口和 Hook，各版本共用模块与界面。

**当前扩展方式是修改源码后重新编译注入器和 DLL。** `.inc` 是编译期映射文件，不是运行时插件；把新文件放进目录不会自动增加可注入版本。只补 mapping 适用于调用方式和行为确实兼容的部分；字段变成方法、容器结构改变、事件位置改变或渲染后端改变时，还需要 C++ 适配。

## 1. 先确定要支持的客户端

版本应由“Minecraft 版本 + 客户端/加载器 + 实际运行时命名 + 渲染方式”共同确定。例如，同为 1.8.9，Lunar、Forge 和原版不能直接视为同一个 profile。

当前目录见 [ClientProfiles.inc](../../shared/ClientProfiles.inc)：

| Key | 实现参考 | 窗口/渲染 |
|---|---|---|
| `forge-1.20.1` | `forge1201.inc`、`Game*.cpp` 中的 Forge 实现 | GLFW / OpenGL |
| `lunar-1.8.9` | `Lunar189.inc`、`LunarRuntime.cpp` | LWJGL 2 / OpenGL |
| `minecraft-26.3` | `minecraft263.inc`、`Minecraft263*.cpp` | SDL3 / OpenGL 或 Vulkan |
| `easecation` | `easecation.inc`、EaseCation 适配及公共现代接口；见 [说明](easecation.md) | NeoForge 21.8.52 / GLFW / OpenGL |

选择结构最接近的实现参考，但逐项核对目标客户端。上述表格描述已有实现入口，不代表每个模块已经在所有版本完成游戏内验收。

接入前记录：客户端构建号、Java 版本与位数、目标类/JAR 来源、命名空间、类加载器、窗口库、图形后端。当前注入构建以 Windows x64 为目标；还要确认目标 JVM 支持所需的 JNI/JVMTI 和类重转换能力。

可以使用 Recaf 查看目标类，或用匹配目标 class 版本的 JDK 执行：

```powershell
# 替换为实际路径和类名；这是离线查看命令。
& 'C:\path\to\jdk\bin\javap.exe' -classpath 'C:\path\to\client.jar' -p -s -c net.minecraft.client.Minecraft
```

Forge 的 `m_..._` / `f_..._`、开发环境的可读名称、Lunar 运行时转换后的名称可能不同。客户端会改写类时，原始 JAR 只能作为参考，最终必须核对 JVM 实际加载的成员和字节码。使用 Recaf 时保留用户指定的工作区，不擅自替换已打开的 JAR。

## 2. 理解接入链路

```text
shared/ClientProfiles.inc
  → WinUI / CLI 选择 profile ID
  → shared/Bootstrap.h 中的 NovaBootstrap
  → Loader.cpp → compat::Select / Initialize
  → mapping + 版本运行时 + 游戏接口分发
  → Java bridge / 字节码 Hook / 渲染 Hook
  → 公共模块、ClickGUI、HUD
```

| 接入层 | 文件入口 | 新版本要完成的工作 |
|---|---|---|
| 版本目录 | [shared/ClientProfiles.inc](../../shared/ClientProfiles.inc) | 增加唯一符号、ID、key、显示名称 |
| Profile | [Profile.h](../src/minecraft/compat/Profile.h)、[Profile.cpp](../src/minecraft/compat/Profile.cpp) | 注册 mapping、能力、后端，增加初始化与退出分支 |
| 映射 | [mappings](../src/minecraft/compat/mappings) | 每版本一个文件，集中名称、描述符和改写规则 |
| 游戏接口 | [src/minecraft/java/api](../src/minecraft/java/api)、[src/minecraft/java/game](../src/minecraft/java/game) | 将 Minecraft `game_*.h` 接口分发到新版本实现 |
| 版本语义 | [LunarRuntime.cpp](../src/minecraft/compat/lunar189/LunarRuntime.cpp)、[Minecraft263Game.h](../src/minecraft/compat/minecraft263/Minecraft263Game.h) | 参考已有适配，处理结构和调用语义差异 |
| Hook 生命周期 | [Loader.cpp](../src/core/Loader.cpp)、[Edits.cpp](../src/minecraft/hooks/Edits.cpp)、[Hooks.cpp](../src/minecraft/hooks/Hooks.cpp)、[Transformer.cpp](../src/minecraft/hooks/Transformer.cpp) | 选表、注册 native、改写校验、卸载恢复 |
| 渲染与输入 | [OverlayHost.cpp](../src/minecraft/host/render/OverlayHost.cpp)、[VulkanOverlay.cpp](../src/ui/backend/VulkanOverlay.cpp) | 窗口、帧回调、输入捕获、后端资源 |
| 功能集合 | [Registry.cpp](../src/minecraft/modules/Registry.cpp) | 核对模块依赖与能力，保持公共模块集合 |
| 编译 | [minecraft/compat/CMakeLists.txt](../src/minecraft/compat/CMakeLists.txt)、[minecraft/java/CMakeLists.txt](../src/minecraft/java/CMakeLists.txt) | 将新增 `.cpp` 登记到所属子系统；目录规则见 [项目结构](project-structure.md) |

以下用 `CustomClient` / `custom-client` 举例。**它们是假想占位符，不是已经支持的新版本，也不是可以直接编译运行的完整补丁。**

## 3. 注册注入器版本选项

在 `shared/ClientProfiles.inc` 追加一行，例如：

```cpp
NOVA_PROFILE(CustomClient, 4, "custom-client", "Minecraft Custom Client")
```

示例 ID `4` 仅在尚未被使用时可用。符号、ID、key 均保持唯一，不修改已有 ID，避免旧注入器把同一个数值解释为另一个客户端。

注意此文件同时被 C/C++ 预处理器和 WinUI 文本解析器读取：

- 每条定义完整地放在一行，沿用现有格式。
- 空行允许；当前 WinUI 解析器不接受注释行、行尾注释或多行宏。
- WinUI 将文件作为 `Nova.ClientProfiles` 资源嵌入。修改后必须重新编译 UI，不能只替换 EXE 旁的 `.inc`。
- CLI 同样在编译时读取目录；发布时应配套更新 WinUI、CLI 和 DLL。

[Bootstrap.h](../../shared/Bootstrap.h) 自动从目录生成 `NovaClient_CustomClient`。只增加 profile 无需改变 bootstrap 内存布局或 ABI。不要为新版本复制另一套注入流程。

## 4. 为版本建立一个完整 mapping 文件

建议新增 `Client/src/minecraft/compat/mappings/customclient.inc`。每个版本只保留一个映射文件，用不同宏分区供各调用点重复包含；**不要添加 `#pragma once` 或全文件 include guard**。

首先实现基础 profile 分区。下例所有 `REPLACE_*` 都必须换成实际运行时名称：

```cpp
#if defined(NOVA_CUSTOMCLIENT_PROFILE)
#include "minecraft/compat/Profile.h"
namespace nova::compat {
inline constexpr GameMapping CustomClientMapping{
    "REPLACE_SINGLETON_METHOD",
    "REPLACE_PLAYER_FIELD", "LREPLACE_PLAYER_INTERNAL_NAME;",
    "REPLACE_WORLD_FIELD", "LREPLACE_WORLD_INTERNAL_NAME;",
    "REPLACE_SCREEN_FIELD", "LREPLACE_SCREEN_INTERNAL_NAME;"
};
}
#else
#error "Select a CustomClient mapping section before including this file"
#endif
```

这七项仅供启动时查找 Minecraft 单例、玩家、世界、屏幕，**不是完整功能 mapping**。继续按目标接口增加实体、背包、输入、选项、交互、网络、皮肤、HUD、Backtrack 和 Hook 分区。可参考：

- [forge1201.inc](../src/minecraft/compat/mappings/forge1201.inc)：通过 `NOVA_FORGE1201_*` 分区提供各源文件使用的映射。
- [Lunar189.inc](../src/minecraft/compat/mappings/Lunar189.inc)：类、字段、方法表参与句柄声明、解析及释放。
- [minecraft263.inc](../src/minecraft/compat/mappings/minecraft263.inc)：集中成员目录与改写表，由 Access/Game 适配层使用。

分区宏不是自动注册机制。增加分区后，需要在使用处定义宏、包含文件、展开表并取消宏定义。

每条 JNI 绑定至少核对 owner、成员名、描述符、静态/实例属性和调用方式：

| JNI 描述符 | 含义 |
|---|---|
| `I` / `J` / `F` / `D` / `Z` | int / long / float / double / boolean |
| `V` | 方法返回 void |
| `Ljava/lang/String;` | String 对象 |
| `[I` / `[Ljava/lang/String;` | int 数组 / String 数组 |
| `(ID)Z` | 参数 int、double，返回 boolean |
| `()Lnet/minecraft/client/Minecraft;` | 无参数，返回 Minecraft 对象 |

对象描述符中的内部类名用 `/`，不要与类加载辅助函数使用的点分名称混淆。重载参数必须精确匹配，字段不能用 `GetMethodID` 查找，静态成员不能按实例成员调用。

## 5. 注册 Profile 并消除错误的默认分支

在 `Profile.cpp` 中包含基础分区：

```cpp
#define NOVA_CUSTOMCLIENT_PROFILE
#include "minecraft/compat/mappings/customclient.inc"
#undef NOVA_CUSTOMCLIENT_PROFILE
```

然后在 `profiles[]` 定义之前提供与目录符号对应的常量，例如：

```cpp
// 仅演示符号命名；后端必须根据目标客户端选择和实现。
constexpr RenderBackend CustomClientRenderer = RenderBackend::Glfw;
constexpr std::uint32_t CustomClientCapabilities = 0;
```

现有宏会组合出 `CustomClientRenderer`、`CustomClientMapping`、`CustomClientCapabilities`，少一项就无法编译。能力 `0` 只是开发起点，不能据此认为所有依赖游戏接口的模块都会被禁用。

在 `Profile.h/.cpp` 增加 `IsCustomClient()`，比较 `Current().id` 与 `NovaClient_CustomClient`，并接入版本初始化、错误报告和清理。逐项检查以下现有假设：

1. `Initialize()` 目前固定寻找 `Lnet/minecraft/client/Minecraft;`，单例返回类型也固定。若目标主类不同，仅改七项 mapping 不够，需要让这两项也按版本解析。
2. 普通分支从 Minecraft 读取 screen 字段；26.3 则通过 `Minecraft.gui → Gui.screen()`。若目标结构类似后者，应实现对应访问函数，不能把方法名填入字段项。
3. 当前 `renderer == Lwjgl2` 的初始化包含 Lunar Genesis 校验。接入原版/Forge 1.8.9 时，应将 Lunar 专属校验限定到 Lunar，保留目标需要的 LWJGL 校验。
4. `ScreenOpen`、`ChatScreenOpen`、`CaptureMouse`、`RestoreMouse`、`Shutdown` 等也有版本分支，不能只实现启动成功。

**当前代码大量采用“26.3 / Lunar / 否则 Forge”的分发。** 新 ID 会落入 Forge 分支，必须显式接入，而不是依赖默认行为。用以下命令建立检查清单，逐一阅读匹配位置：

```powershell
rg -n 'IsLunar189|IsMinecraft263|Forge1201|RenderBackend::' Client/src
rg -n 'NOVA_HUD_MAPPING|NOVA_BACKTRACK_MAPPING|AllEdits|Select.*Edits' Client/src
```

尤其不要遗漏 [GameHud.cpp](../src/minecraft/java/game/GameHud.cpp) 和 [GameBacktrack.cpp](../src/minecraft/java/game/GameBacktrack.cpp)：它们分别包含现有三份映射表，并通过版本判断选表。需要包含新表并修改选择逻辑，新增 `.inc` 不会被自动发现。

## 6. 实现公共游戏接口

新增 `CustomClientRuntime.h/.cpp` 或按实体、背包、动作等拆分实现，在 `Client/CMakeLists.txt` 注册所有新 `.cpp`。版本映射仍集中在同一个 `.inc`；运行时代码可以按职责拆分。

以 `Client/src/minecraft/java/api/game_*.h` 为接口契约，在 `Client/src/minecraft/java/game/Game*.cpp` 增加显式分发。模块继续调用公共接口，避免为每个版本复制一套模块和设置。

| 接口领域 | 必须核对的语义 |
|---|---|
| 实体与旋转 | 当前/上一 tick 位置、眼睛位置、碰撞箱、角度、插值时间、实体 ID 与世界生命周期 |
| 输入与动作 | 使用/攻击键、跳跃和冲刺、射线命中、动作执行线程、现代输入结构 |
| 物品与容器 | 空物品表示、槽位号、快捷栏与容器索引转换、容器 ID、点击类型、副手支持 |
| 网络 | 包类型、数值单位、方向、发送/处理边界、重放路径和确认顺序 |
| HUD 与皮肤 | 文本/组件读取、玩家身份、延迟与时间信息、皮肤资源和图形后端纹理 |

名称相同不代表语义相同。例如旧版实体增量可能使用 `1/32` 单位，其他版本可能使用 `1/4096` 或增量编解码对象；必须根据目标实现解码。不能复用字段名后仍沿用旧的缩放系数。

JNI 生命周期也属于适配工作：

- `JNIEnv*` 只属于当前线程，不缓存给其他线程使用；沿用项目的线程附加工具。
- 长期保存的 Java 对象/类使用全局引用，退出世界、失败回滚和卸载时按所有权释放。
- 当前 `compat::ClassLoader(env)` 返回新的全局引用，调用方需要 `DeleteGlobalRef`；`Minecraft(env)`、`LocalPlayer(env)` 返回局部引用。
- 解析失败检查并处理 Java 异常，记录失败 owner/name/descriptor；关键接口缺失应停止初始化，不以清除异常后继续调用空句柄代替适配。
- 反射式加载路径不能直接套用普通 MSVC 静态 `thread_local`；涉及重入保护时参考现有 Win32 TLS 实现与释放流程。

## 7. 接入 Java Hook 与事件

只读 JNI 成功仍不足以驱动模块，必须接入事件。参考 [GameEvents.h](../src/minecraft/events/GameEvents.h)，核对事件发生位置、次数、线程和取消/修改效果。

需要联动修改：

1. `Edits.h/.cpp`：增加目标版本改写表和选择入口，让 `AllEdits()` 返回新表。
2. `Loader.cpp`：选择新改写表，进入桥接安装流程。目前条件为 Lunar、26.3 或 `Forge1201Events`，不能只增加目录项就期待执行。
3. `Hooks.cpp`：接入 `Prepare`、native 名称/签名/数量、回调实现和 `Release`；必要时调整 [HookBridge.cpp](../src/minecraft/java/HookBridge.cpp) 的桥接生成。
4. `Transformer.cpp`：将新 profile 纳入必要规则匹配数和失败校验。目前部分严格检查只针对 Lunar/26.3。
5. 初始化失败与卸载：还原改写类、停止回调、等待在途调用退出、注销 native 并释放引用，接入现有生命周期，不自行绕过卸载保护。

不要将 `Forge1201Events` 当作任意新版本的“已接入事件”开关。它目前还参与 Forge 安装路径，应明确增加目标版本的路由。

重点核对这些事件契约：

- `GameTick` 的 Pre 阶段应处于所需交互/移动之前；PlayerTick、输入更新、Motion 前后必须与实际调用顺序一致。
- 包事件区分客户端连接的 Incoming/Outgoing；单人 JVM 中内置服务端的流量不能误触发客户端模块。
- 取消包必须作用于实际发送/处理入口，重放要避免再次入队，并保留应有的 Java 异常处理。
- `AttackEntityEvent` 的方法头位置可能已处于一次攻击序列中间。1.8 先挥手后攻击，现代实现顺序不同；不要在旧版该回调中刷新队列而拆开原始顺序。现有 Backtrack 将目标切换处理推迟到下一次 GameTick。

字节码规则必须匹配目标字节码，而非仅匹配反编译 Java：确认指令类型、方法描述符、ordinal 和预期命中数量。现有 `Append` 在 `atHead=false` 时插入到最后一个 return 前，并不覆盖每个提前返回；有多出口的方法要核对覆盖范围或改用合适的包装方式。改写后验证栈、局部变量、分支和异常表。

## 8. 渲染、输入与公共模块

`RenderBackend` 当前只有 `Glfw`、`Lwjgl2`、`Sdl`，它们主要描述窗口接入路径。**选中一个枚举值不等于已经支持该图形后端。**

- GLFW/LWJGL：核对窗口、OpenGL 上下文、帧入口、Win32 输入与鼠标抓取。
- SDL：当前 `GlHook.cpp` 的路径仍调用 26.3 专用窗口/GPU 接口，新 SDL 客户端需要扩展分发。
- Vulkan：同时适配 `VulkanOverlay.cpp`、交换链生命周期、帧资源和皮肤纹理；不能把 OpenGL 的 `GLuint` 直接当作 Vulkan 纹理句柄。
- HUD 数据跨线程传递时参考已有 CPU 快照方式；验证游戏窗口调整尺寸、切换世界和后端资源重建。
- 验证聊天打开时不会误开 ClickGUI、鼠标捕获/恢复正常、子面板展开时 ESC 仍可关闭。

保持用户可见的公共模块集合，以 `Registry.cpp` 为准。能力位涉及 Settings、PlayerEffects、ClickInput、RotationEvents、NetworkEvents、BasicPlace 等，但并不是完整依赖图：部分模块无条件注册。因此不能仅凭“界面有模块”判断功能已经接入，也不能仅靠清空能力位保证开发中的 profile 可安全使用。

版本本身不存在的机制可以保留设置并有意无效果，例如现代版本没有 1.8 的剑格挡；应写明原因。目标版本存在的功能却因缺映射而失效，属于未完成适配，不能按“版本差异”处理。

## 9. 构建注入器与同一份 DLL

以下是本工作区的 PowerShell 命令；其他机器先修改仓库与 Visual Studio 安装路径。需要 MSVC C++ 工具、Windows SDK、CMake，以及 WinUI 项目要求的 .NET 8 / Windows App SDK 构建依赖。Java 离线验证需使用能读取目标 class 版本的 JDK。

```powershell
Set-Location D:\AI\Minecraft\Nova
$novaCmake = 'D:\VS2026\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'

# 初次配置；本工作区现有缓存使用 Visual Studio 18 2026。
& $novaCmake -S Client -B Client/build-vs2026 -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
& $novaCmake --build Client/build-vs2026 --config Release
if ($LASTEXITCODE -ne 0) { throw 'DLL build failed' }

& .\tools\build-injector.cmd
if ($LASTEXITCODE -ne 0) { throw 'Injector build failed' }
```

`tools/build-injector.cmd` 当前固定使用 `D:\VS2026\VS2026` 的工具链。换机器时调整该脚本；已有构建目录若使用其他生成器，另建目录，不混用不同生成器的缓存。

本配置 DLL 产物：`D:\AI\Minecraft\Nova\Client\build-vs2026\Release\NovaClientBase.dll`。原生 CLI 为 `out\injector\native\NovaInjector.exe`；构建脚本同时发布 `build\Injector.WinUI.exe`，该文件已包含 CLI 与运行依赖，分发注入器只需此 EXE。客户端 DLL 仍单独选择。构建和独立启动检查见 [注入器说明](../../injector/README.md)。

由用户启动目标客户端，使用重新构建的 UI 选择新 profile 后注入。CLI 等价命令如下，**仅供完成适配后的人工验证，替换 PID 与 key 后才执行**：

```powershell
& .\out\injector\native\NovaInjector.exe 12345 'D:\AI\Minecraft\Nova\Client\build-vs2026\Release\NovaClientBase.dll' custom-client
```

当前 CLI 要求目标进程名为 `java.exe` 或 `javaw.exe`，并要求 DLL 绝对路径。自定义 JVM 宿主 EXE 还需扩展进程识别并核对实际 JVM，不能只给版本下拉框加一项。

## 10. 按层验证，记录实际覆盖范围

先做离线验证，再由用户注入和进行游戏测试：

| 阶段 | 验证内容 | 不能由此推断的结论 |
|---|---|---|
| 映射审计 | 目标类中的 owner/name/descriptor/staticness 与表一致 | JVM 运行时转换后仍一定相同 |
| JNI 初始化 | 正确 profile 成功解析；错误版本应明确拒绝 | 所有动作和事件已经可用 |
| Hook 离线验证 | 每条规则命中数量正确；改写类通过 JVM `-Xverify:all` 与针对性执行 | 已覆盖游戏全部分支与线程 |
| Release 构建 | DLL、CLI、WinUI 均成功 | 注入和绘制已经成功 |
| 首帧与输入 | 菜单/HUD 绘制，输入、鼠标、皮肤、窗口调整正常 | 背包、网络、移动模块行为正确 |
| 游戏功能 | 进入/退出世界、实体插值、背包、交互、网络模块逐项测试 | 未测试版本或后端也通过 |
| 生命周期 | DEL 卸载、改写还原、再次注入、失败回滚 | 仅有“发出卸载请求”就等于卸载成功 |

可复用的测试参考：

- [test/minecraft263](../test/minecraft263)：映射审计、接口探针、Hook 改写与 Java 验证。
- [test/bytecode](../test/bytecode)：字节码编辑器测试。
- [test/combat/backtrack](../test/combat/backtrack)、[test/combat](../test/combat)：公共模块和事件顺序回归。
- [test/opengl](../test/opengl)、[test/vulkan](../test/vulkan)：图形后端相关检查。

这些脚本不是通用的新版本测试器。例如现有 `audit_mapping.py` 使用固定版本的映射和样本，不能只把任意 `.inc` 文件名传进去。为目标版本准备匹配的类样本、适配脚本与断言，并记录跳过项。离线 mock 通过不算游戏内验证。

当前 Forge/Lunar 在原生渲染 Hook 安装后报告就绪；26.3 使用 `WaitForFirstFrame(15000)` 等待首帧。新版本应明确就绪条件。窗口没有绘制或被最小化时，不能把首帧超时直接归因于 mapping。

## 11. 常见问题定位

| 现象 | 优先检查 |
|---|---|
| UI 没有新选项 | 是否重编译并运行新的 WinUI；目录行是否符合解析格式 |
| 编译找不到 `CustomClientMapping` 等符号 | 目录符号与三个常量命名是否一致，分区是否被包含 |
| 选新版本却访问 Forge 成员 | `Profile`、`Game*.cpp`、HUD/Backtrack、Hook 是否仍走默认 Forge 分支 |
| LWJGL 2 客户端初始化失败 | 是否错误地要求非 Lunar 客户端存在 Genesis |
| `NoSuchMethod` / `NoSuchField` | 实际命名空间、owner、签名、静态性、类加载器 |
| 注入成功但无界面 | 首帧是否实际到达、窗口与图形后端、配置是否启用对应 HUD |
| 界面正常但功能不响应 | bridge 是否安装、规则数量、tick/input/motion/network 事件是否触发 |
| HUD 实体位置错位或不同步 | 上一 tick 位置、当前坐标、相机与插值时间是否同一语义 |
| 容器读取正常但不拿物品 | 当前容器 ID、槽位换算、点击签名、动作执行线程 |
| 卸载后崩溃或不能再次注入 | 在途 Hook、Java/native 桥接、全局引用、TLS、图形资源是否按顺序清理 |

必要时检查 Transformer 的 `%TEMP%\nova-hook-<stage>\` 类转储，比较改写前后的真实字节码。日志应包含选中的 key、失败绑定、改写数量和初始化阶段，不以“加载 DLL 成功”替代完整链路证据。

## 12. 提交前清单

- [ ] 新 profile 的符号、ID、key 唯一；旧 ID 未改变。
- [ ] 每版本一个 mapping 文件，所有使用点已接入，没有依赖自动扫描。
- [ ] 初始化能区分目标客户端，关键接口缺失时明确失败。
- [ ] 游戏接口、HUD/Backtrack、Hook 与渲染分发均不会误落入 Forge。
- [ ] 公共模块集合保持一致，有意无效果的版本机制已说明。
- [ ] 新源文件已加入 CMake，注入器和 DLL 一起重建。
- [ ] 离线验证记录目标类来源、JDK、预期命中数、通过项和跳过项。
- [ ] 用户完成目标版本/后端的首帧、功能、退出世界、卸载及再次注入验证。
- [ ] 更新 [多版本支持概览](multi-version-support.md)，新增目标版本适配说明，区分已实现与已实测。

若以后要实现“用户只放一个外部 mapping 即可扩展”，还需要另行设计运行时映射格式、模式校验、版本探测、适配策略分发、Hook 规则加载及生命周期管理。数据映射可以减少名称变化带来的代码修改，但不能自动解决不同版本之间的行为差异；这些目前仍由源码适配层承担。
