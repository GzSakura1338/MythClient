# EaseCation 适配

注入器显示名称为 **EaseCation**，profile ID 为 `5`，配置目录 key 为 `easecation`。
与其他版本共用 `NovaClientBase.dll`，不用单独分发第二个 DLL。
此分支只适配原来的 Minecraft **1.21.8 / NeoForge 21.8.52** 环境。

## 目标环境和分析来源

- 用户提供的原始 JAR：`E:/MCLDownload/Game/.minecraft/versions/1.21.8/1.21.8.jar`。
- SHA1：`a19d9badbea944a4369fd0059e53bf7286597576`，与 Mojang 官方 1.21.8 客户端一致。
- `version.json`：协议 772、Java 21、class major 65。
- 同目录启动配置：NeoForge 21.8.52，BootstrapLauncher，GLFW / OpenGL。
- Recaf 保持原 JAR 工作区，只读检查 Main、KeyboardInput、LocalPlayer.sendPosition、击退包与处理方法、使用物品时发送旋转的字节码。
- 同目录实际安装的 `neoforge-21.8.52-client.jar` 用于离线校验运行时名称和 Hook 改写，未修改原文件。

原始 JAR 是混淆名称（例如 `fue`、`gwi`、`gwh`、`grl`）；NeoForge 加载的类采用 Mojang 名称。
**此 profile 针对已校验的 NeoForge 环境，不是对任意混淆原版、Fabric 或未知 EaseCation 构建的通用承诺。**
版本和基础接口不匹配会停止初始化，所有必需 Hook 未完整安装时不会继续启用模块。

## 结构

- `shared/ClientProfiles.inc`：注入器和 DLL 共用的版本目录。
- `src/minecraft/compat/mappings/easecation.inc`：该版本的 profile、类型别名、成员差异、背包/相机/皮肤/队伍/网络绑定，以及完整 Hook 选表。
- `src/minecraft/compat/easecation/`：命名转换、版本检查和 screen 生命周期。
- `UsesModernRuntime()`：共用现代游戏接口的分发；`IsMinecraft263()` 和 `IsEaseCation()` 保留严格的版本判断，不能互相代替。
- 同名同签名成员和公共 Hook 复用现代目录。方法的参数、返回类型或语义改变时使用显式 C++ 分支，不通过修改描述符掩盖 JNI 调用类型不一致。

主要差异：screen 是字段；资源名称为 ResourceLocation；背包点击使用 ClickType；相机通过 GameRenderer/Camera 读取；玩家皮肤直接返回纹理位置；挥手使用 ServerboundSwingPacket；击退包使用 8000 固定点分量及 lerpMotion(DDD)；实体位置增量、队伍颜色也独立处理。
手部渲染 Hook 位于 ItemInHandRenderer，攻击 Hook 位于 Player.attack，客户端帧 Hook 位于 Minecraft.runTick。
沿用现代版本的公共模块集合；这不会给现代 MC 添加旧版剑格挡等原版不存在的机制。

## 验证和使用

```powershell
# 检查实际安装的 NeoForge 类文件、成员和重写结果；不启动/注入游戏。
Client/test/easecation/run.ps1
# 检查发布的单文件注入器及 EaseCation 版本选项；不执行注入。
tools/test-injector-single-file.ps1
```

检查包括 JNI 绑定、18 个目标类（含代理连接）的 Hook 匹配、原生回调描述符，以及改写后分支/异常表/StackMap 边界；构建还检查 DLL 的静态 TLS。
这些是离线检查，不等价于游戏内功能验收。

安装了 Mixin 的客户端还需覆盖运行时类字节码：
`Client/test/easecation/run.ps1 -RuntimeDumpDirectory "$env:TEMP/nova-hook-in"`。
2026-09-21 的实际 LocalPlayer.tick 在末尾多了 Mixin 回调，最后的 RETURN
不再是原有 StackMapTable 的分支目标。EaseCation 的 playerTick 取消规则使用
独立入口 RETURN 和入口状态帧，避免依赖末尾局部变量/回调布局；其他 profile
保持原有 guard 规则。`Client/test/easecation/run_entry_guard.ps1` 使用已有编译
fixture，在独立 JVM 中验证取消/正常执行及 StackMap，不创建 Java 源文件。

使用 `build/Injector.WinUI.exe` 选择 **EaseCation**，DLL 选择
`Client/build-vs2026/Release/NovaClientBase.dll`。进入游戏后的 HUD、交互、移动、容器和网络行为由用户实际注入验证。
本次不创建 Java 源文件、不改写游戏安装文件、不自动注入游戏。

## 启动器 C++ 运行库兼容

该启动器的 JDK 22 目录带有旧版 `msvcp140.dll`（14.0.24215.1）。
新版 MSVC 默认的 constexpr mutex 初始化与其不兼容，会在锁定时出现
`0xc0000005`，甚至早于 Nova 初始化日志。

仍使用同一个 DLL，不启用全局 `_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR`。
`core/RuntimeImports.h` 在反射加载器绑定导入时检查 bootstrap：只有合法的
EaseCation profile 才将 Nova 的 MSVC 运行库导入绑定到 Windows 系统目录中的
运行库（此环境已验证版本 14.51.36247.0）。必须使用绝对路径，避免再次命中
已加载的同名旧 DLL。其他 profile、非 MSVC 导入和游戏自身的导入保持原路径。
所需系统运行库加载失败时停止初始化，不回退到已知不兼容的启动器副本。
这个分支在 CRT 初始化之前执行；直接通过 Windows LoadLibrary 加载不走此分支，
EaseCation 应使用注入器的反射加载路径。

`Client/test/loader/run_mutex_compatibility.ps1` 在隔离的测试目录验证原始构建的
崩溃、系统运行库兼容性及正式反射加载器的版本分流。测试预加载旧运行库，
确认 EaseCation 的互斥锁/条件变量可工作、其他 profile 仍绑定原运行库，
全程不启动或注入游戏。

## Ixeris 窗口线程

此客户端包含 Ixeris，窗口事件线程与 OpenGL 渲染线程可能不同。
`SetWindowSubclass` 必须在窗口所属线程调用，直接从 swap 回调安装会失败。
仅 EaseCation 的 `Win32Surface.Bind` 启用异步线程切换：通过临时的线程消息钩子
在窗口线程安装/移除子类，渲染线程下一帧检查结果。跨线程窗口输入排队后在
渲染线程进入 ImGui；卸载等待子类移除完成，避免遗留指向 DLL 的回调。
其他 profile 仍使用原来的同线程绑定与消息处理。测试入口为
`Client/test/render/surface/run.ps1`，包含 OpenGL 3.2/4.6 恢复与跨线程生命周期测试。
