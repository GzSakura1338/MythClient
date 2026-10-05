# Forge 1.20.1 注入初始化崩溃：2026-10-03

## 已确认的本次原因

注入后的初始化进入代理钩子准备阶段，首次锁定 `ProxyService.cpp` 的
`std::mutex` 时，游戏自带的旧版 MSVC C++ 运行库与 Nova 编译器生成的
mutex 初始结构不兼容。旧运行库解引用空的锁实现指针，导致进程退出。
注入器随后无法通过 `ReadProcessMemory` 读取初始化状态，报告 error 299。

本次是 Forge 1.20.1，不能按历史 Lunar 崩溃归因。

## 证据

- 最新 `nova-base.log` 在 `Profile: forge-1.20.1 resolved Minecraft, player,
  world, screen and class loader` 后停止；尚未记录 Bridge 安装或模块注册。
- Windows Application Error：2026-10-03 17:16:55，PID 46672，异常
  `0xC0000005`，模块 `msvcp140.dll + 0x18C34`。
- 宿主 Java：`D:/MCLDownload/ext/jre-v64-220420/jdk17/bin/javaw.exe`，17.0.2。
  实际加载的 `msvcp140.dll` 位于同目录，版本 `14.0.24215.1`。
- 转储：`C:/Users/13366/AppData/Local/CrashDumps/javaw.exe.46672.dmp`。
  异常线程 9756。使用 ExceptionStream 内保存的异常 CONTEXT，而非线程列表
  中 WER 收集时的 CONTEXT。异常指令 `mov rbx, qword ptr [rax]`，RAX=0；
  前一条指令读取 mutex 的 `+8` 字段。
- Nova 映像基址 `0x184E40000`；转储保存的 1959 个映像片段全部与当前
  修复前 2026-10-02 22:06:58 构建的 `NovaClientBase.dll` 匹配。
  基于该构建当时对应的 MAP：
  `+0x29FB65` 对应 `proxy::SetSupported +0x15`，`+0x517D5` 对应
  `hook::Prepare +0x15`，`+0xC38B5` 对应 `bridge::Install +0x2F5`，
  `+0x45A85` 对应 `minecraft::RunHost +0x4D5`。这些是异常栈中的返回地址。
- R14=`0x1868D6CD0`，对应映像 `+0x1A96CD0`，恰为
  `ProxyService.cpp` 的全局 mutex 符号。
- 16:52 的 PID 14768 转储具有相同崩溃指令、相同返回地址 RVA、相同 mutex
  RVA；其保存的 1959 个映像片段也全部匹配当前 DLL。

## 代码路径

`RunHost -> bridge::Install -> hook::Prepare -> proxyadapter::Initialize ->
proxy::SetSupported -> std::mutex::lock -> 旧版 msvcp140`。

代理是否启用不影响这次初始化：`Initialize` 总会调用 `SetSupported`，
该函数总会锁定代理状态。因此关闭 Proxy 或 Scaffold 不能消除根因。

`Client/src/core/RuntimeImports.h::NovaUsesSystemRuntime` 当前只给 EaseCation
配置启用 Nova 自身的系统 MSVC 运行库导入；Forge 配置仍使用宿主旧运行库。
这解释了已有兼容修复没有覆盖本次运行。

## 独立复现

运行现有原生测试：

```powershell
& Client/test/loader/run_mutex_compatibility.ps1 `
    -RuntimeDirectory 'D:/MCLDownload/ext/jre-v64-220420/jdk17/bin'
```

新版编译器生成的 mutex 探针与游戏自带运行库组合，退出码
`-1073741819`（`0xC0000005`）；同一探针改用系统运行库通过。
已有 EaseCation 反射导入测试也通过。

该测试对其他配置目前只检查导入归属，未真正执行 mutex 加锁，不能把
其 profile PASS 解释成 Forge mutex 兼容通过。

## 修复方向与本次范围

应让 Forge 配置也使用兼容的运行库导入，并补充旧宿主运行库下真正执行
锁定、递归锁与条件变量的反射加载回归。沿用 Nova 自身 IAT 的隔离机制，
无需替换游戏 JRE 文件。

用户提供的实例目录为 `D:/MCLDownload/Game/.minecraft/versions/1.20`。
其中 `1.20.json` 的实际版本 ID 为 1.20.1，Java 主版本为 17；该实例
`logs/latest.log` 确认 Forge 47.3.0、Eclipse Adoptium 17.0.2+8。

随后修复：将 Forge1201 加入现有系统运行库导入条件。只有 Nova 自己的
MSVC 导入改用系统副本，宿主已加载的旧运行库保留，不修改 JRE 或游戏文件。
无法加载所需系统副本时仍明确拒绝初始化，不回退到旧副本。

测试现已对 Forge 与 EaseCation 反射加载执行普通锁、递归锁、条件变量超时、
双线程累计 2000 次加锁以及持锁抛出 C++ 异常后的 RAII 解锁；其他配置检查
仍沿用宿主导入。覆盖配置 ID 0..8、非法 bootstrap 和系统导入失败路径。
使用上述实例的 JDK17 自带运行库复现旧探针访问冲突，修复路径全部通过。

Release DLL 于 2026-10-03 17:32:18 构建，28,355,584 字节，输出为
`Client/build-vs2026/Release/NovaClientBase.dll`，静态 TLS 检查通过。
`run_reflective_unwind.ps1 -ProductionDll` 也通过：16 次反射加载／异常／卸载
循环，以及正式 DLL 的错误宿主拒绝、线程退出与自卸载检查。
尚未对实际游戏进程重新注入；以上为独立原生回归结果。

16:26、16:43 的 ntdll 异常以及早先 JVM 编译线程异常没有混入本次结论。

## 17:34：注入成功但无界面（PID 51096）

实际游戏重新注入后，旧 CRT 崩溃未再发生。日志确认 48 个桥接钩子、
64 个模块及 18 个类转换成功，OpenGL 3.2 / GLSL 1.50 渲染初始化成功。
随后却出现 `platform=0 pending=0 renderer=1`，并自动完整卸载。

窗口线程为 56136，渲染线程为 55988。Forge 未启用已有跨线程窗口绑定，
直接从渲染线程调用 `SetWindowSubclass` 失败。日志里的 `error=127` 来自
该路径读取的线程 LastError，不能据此认定缺少某个 OpenGL 或 Win32 导出。
已有原生窗口测试确认同样的直接跨线程绑定失败，异步窗口线程绑定成功。

本次将 Forge 接入已有异步绑定与输入消息队列；ImGui 输入仍在渲染线程
处理，窗口回调在窗口线程安装和移除，不改变 Forge 的 Minecraft JNI 适配器。
初始化、后续窗口重新绑定及失败检查使用同一条件。

另外，Forge 之前在安装 swap hook 后立即报告 Ready，导致注入器在平台
初始化失败之前已返回退出码 0。现在必须等到首个有效 overlay 帧完成，
才报告成功；初始化失败或 15 秒未出现首帧会报告失败。

Release DLL 已于 2026-10-03 17:42:53 重新构建，静态 TLS 检查通过。
跨线程测试覆盖安装完成与首帧检查的时序、8 次安装／解绑、待安装时取消，
以及保留第三方窗口子类，全部通过。此修复版尚未在游戏内重新注入验证。

## 17:45：首帧字体上传崩溃（PID 18452）

报告为 `D:/MCLDownload/Game/.minecraft/hs_err_pid18452.log`，原生转储为
`C:/Users/13366/AppData/Local/CrashDumps/javaw.exe.18452.dmp`。
这次日志已到 `frame 1 platform=1 renderer=1`，说明输入绑定已成功。
真正的访问冲突发生在渲染线程的 `nvoglv64.dll+0xb9e380`，读取
`0x17c7eb000`。错误 299 是进程崩溃后注入器无法读取初始化状态的结果。

使用 17:42:53 版 DLL 的链接映射和转储中的原始故障栈定位：

```text
HookSwap+0x23
RunFrame+0xdeb
ImGui_ImplOpenGL3_RenderDrawData+0xce
ImGui_ImplOpenGL3_UpdateTexture+0x10f
glTexImage2D -> NVIDIA texture upload -> invalid CPU memory read
```

手工映射的 Nova 基址为 `0x179cc0000`，纹理上传返回地址为
`0x17a00b07f`（RVA `0x34b07f`）。该调用正在创建动态字体图集，
不是窗口线程消息回调，也不是旧 MSVC mutex 崩溃。

现有 `PixelUnpackGuard` 只在现代 GLFW JNI 配置下启用，漏掉了 Forge。
ImGui 的动态纹理上传会设置 alignment 和 row length，但不清除宿主的
pixel-unpack buffer、skip rows、skip pixels；遗留偏移能让 CPU 图集
上传越界，绑定 PBO 时还会把 CPU 地址当成缓冲区偏移。

现在所有 OpenGL 帧都在上传前清除这些状态，并在结束时恢复宿主原值。
Vulkan 路径不调用 OpenGL。补齐原生 surface 测试需要的 UI 回调替身后，
测试同时覆盖有／无 PBO 的动态字体上传，每个像素与 CPU 图集一致，
宿主的 PBO、alignment、row length、skip rows、skip pixels 均恢复。
OpenGL 4.6 与匹配游戏的 OpenGL 3.2 core context 全部通过；窗口／上下文
重建、字体／皮肤／封面／模糊缓存恢复及跨线程绑定回归也通过。

Release DLL 于 2026-10-03 17:52:38 更新，静态 TLS 检查通过。
实际游戏仍需重新注入此版验证。
