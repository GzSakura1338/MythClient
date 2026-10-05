# Nova 单文件注入器

分发时只需 `build/Injector.WinUI.exe`，无需携带旁边的运行库、XAML/PRI 资源或独立 `NovaInjector.exe`。保留现有 WinUI 界面、进程列表、版本选择和 DLL 浏览流程。

客户端 `NovaClientBase.dll` 是待注入文件，仍由用户单独选择，不嵌入注入器，以便独立更新客户端。

## HeyPixel 模式（Forge 1.20.1，winmm 启动代理）

选择 `Minecraft 1.20.1 (Forge)` 并开启 **HeyPixel 模式**。选择网易游戏的 `.minecraft` 和新版 `NovaClientBase.dll`，退出使用目标 Java 的游戏，点击 **应用 HeyPixel 模式**。下次从网易启动器启动时，Java 通过 `winmm.dll` 代理加载 Nova；无需选择正在运行的 Java 进程，也不需要进入服务器。

当前官方目录为 `E:/MCLDownload/Game/.minecraft`，对应 Java 为 `E:/MCLDownload/ext/jre-v64-220420/jdk17/bin`。自动识别通过网易注册表和固定磁盘定位游戏目录，再按已确认的运行时布局定位 Java。使用自定义 Java 时，在界面填写实际的 `bin` 目录，必须包含 x64 `java.exe` 和 `server/jvm.dll`。这个 Java 的其他可查询运行进程存在时会拒绝修改；退出游戏后再应用。

安装在 Java 的 `bin` 中放置代理，并在 `bin/nova-heypixel` 内保存内容哈希命名的客户端、UTF-16 `loader.ini`、安装清单和原代理备份。代理只在 java/javaw 的工作目录匹配所选游戏目录时加载客户端；手动启动时也需将工作目录设为该游戏目录。后台加载记录在该文件夹的 `loader-<pid>.log`，Nova 运行日志仍为 `%APPDATA%/.minecraft/nova-base.log`。日志中“映像已加载”不表示全部模块已初始化。

代理转发全部 181 个系统 winmm 导出，使用系统目录绝对路径解析真实 DLL。Nova 映像在启动阶段加载；为兼容 JDK 17.0.2 提前报告 VM 的行为，JNI 初始化等待 Java 加载 GLFW，随后等待 Minecraft 静态初始化和必要接口就绪。之后执行完整 Nova 初始化流程，**不等待远程世界、玩家或进服事件**。普通进程注入保持原行为；旧进服补丁的 DLL 文件名仍保留兼容分支。

原先存在的 `winmm.dll` 会按 SHA-256 备份，不执行也不链接到原代理。此次只参考 DLL 代理加载结构，不包含参考项目的硬件标识修改或模块隐藏。**恢复原版**会还原安装前的 winmm 状态；如果原先没有代理，就移除 Nova 代理。客户端副本与备份保留用于恢复。重复应用相同 DLL 为无操作，更新 DLL 不覆盖最初的代理备份；被外部修改的安装文件或损坏备份会使操作停止。

安装、恢复及目录自动识别均不要求存在 HeyPixel 模组或 mods 文件夹，只要求游戏目录存在且 Java 有效。新安装不修改原版 HeyPixel JAR，也不限定原版模组哈希。若发现之前的 Nova 进服/预加载字节码补丁，则先用 `<游戏目录>/nova-backups/heypixel/<原版SHA256>.jar.bak` 验证并恢复原版，避免重复加载。缺少原版备份或模组还有其他修改时拒绝自动迁移。普通文件操作失败会回滚已修改文件；断电/强制结束造成的跨文件中断不是原子事务，需要保留安装目录和 transaction 备份排查。

使用本次发布的注入器配合新版 DLL；旧的 `Injector.WinUI.Joined.exe` 是之前的进服加载版本，不应继续应用。安装器静态检查 `NovaHeyPixelProxyABI_v1` 导出以拒绝不兼容的旧 DLL。未对真实网易启动器更新后的重新下载行为作运行时保证；更新 Java 后需重新识别目录和应用。

测试：

```powershell
dotnet run --project injector/tests/HeyPixel.Tests.csproj -- <原版HeyPixel.jar> <新版Nova.dll> <全新测试目录>
pwsh -File Client/test/loader/heypixel-proxy.ps1 -Dll <新版Nova.dll> -Proxy out/injector/native/winmm.dll
```

文件系统测试覆盖安装/更新、原代理备份恢复、锁冲突回滚、旧 JAR 迁移和新版原模组兼容；代理生命周期测试克隆独立 Java，使用图形加载标记和最小 Minecraft 夹具，验证 Nova 由代理加载并在无世界时启动初始化。转发测试源码为 `tests/winmm-forwarding.cpp`，编译时包含生成的 `out/injector/native/winmm-exports.h`。这些测试不连接服务器，不代表已验证真实游戏渲染和第三方原生模块兼容。

## 构建

在仓库根目录执行：

```powershell
& .\tools\build-injector.cmd
```

脚本当前使用 `D:\VS2026\VS2026` 的 MSVC/MSBuild，需要相应 C++、.NET、Windows SDK 构建环境。脚本先生成 x64 winmm 转发桩、代理和原生 CLI，再使用 `Standalone` 发布配置，把代理作为资源嵌入，并将 CLI、.NET 和 Windows App SDK 运行依赖打包进一个 EXE。不裁剪 WinUI 程序，避免反射/XAML 所需类型丢失。

- 分发文件：`build/Injector.WinUI.exe`。
- UI 构建暂存：`out/injector/ui/`。
- 单文件专用中间目录：`injector/ui/obj/single-file/`。
- 原生 CLI 构建输出：`out/injector/native/NovaInjector.exe`，已经包含在分发文件里。

单文件中间目录不能与普通构建混用，否则 MSBuild 可能复用缺少单文件 DLL 重定向的旧 WinUI 清单，造成程序启动失败。分发只需要新版注入器和所选客户端 DLL；旧版本 EXE 无需一起分发。

## 运行方式

这是单文件分发，不是完全不落盘运行。.NET 首次启动会将依赖释放到用户临时目录下的 bundle 缓存，后续启动可复用；EXE 所在目录不需要提供这些依赖。缓存空间需足够，首次解包会比后续启动慢一些。程序保持管理员运行清单，正常启动会按 Windows 设置请求提升权限。

## 验证

```powershell
& .\tools\test-injector-single-file.ps1
```

测试仅复制一个 EXE 到新的临时目录，使用独立解包缓存，验证 WinUI 窗口创建和内嵌 CLI 无参数启动时的用法/版本目录输出，然后关闭测试窗口。为避免仅检查界面就请求管理员权限，测试子进程使用 `RunAsInvoker`；分发文件清单不变。测试不执行注入，也不会关闭用户已打开的其他注入器。

测试还通过 UI Automation 验证 HeyPixel 模式、版本切换和关闭状态，可传入 `-Executable <路径>` 检查其他发布目录，传入 `-ExpectedHeyPixelDirectory <目录>` 验证本机自动发现。不会点击应用或恢复。
