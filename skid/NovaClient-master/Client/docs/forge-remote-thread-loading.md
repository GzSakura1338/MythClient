# Forge 1.20.1 远程线程加载

注入器选择 Minecraft 1.20.1 (Forge) 后，在 HeyPixel 开关旁显示
“远程线程加载”。默认关闭，继续使用反射注入；开启时使用 LoadLibraryW。
HeyPixel 是启动代理安装，两项互斥。其他版本与 CS2 的加载方式不变。

CLI：

```text
NovaInjector.exe <pid> <absolute dll path> forge-1.20.1 remote-thread
```

不传最后一个参数或传 `reflective` 都是原来的反射路径。

实现先创建进程专属的加载标记，使普通 DLL 的 DllMain 暂缓自动启动，
然后通过目标进程里的 LoadLibraryW 加载 DLL。加载器枚举目标模块获得
完整的 x64 模块地址，不使用只有 32 位的远程线程退出码作为 HMODULE。
随后调用 DLL 导出的 NovaInitializeRemoteThread，传入原有 NovaBootstrap。
该入口在启动工作线程前将 Nova 自己的 MSVC 导入绑定到系统运行库，
保持 Forge JVM 原有的旧运行库与导入不变，避免旧 mutex ABI 引起崩溃。

目标 API 地址根据其实际所属模块及目标模块基址计算，支持 kernel32 的
转发导出。初始化线程、bootstrap 内存和加载路径都有独立生命周期。
挂起或超时的加载保留仍可能被使用的远程参数，并报告需要重启，
不会结束游戏线程。成功加载使用 Windows 的普通 DLL 与异常展开管理；
熔断后的既有自卸载逻辑按 MEM_IMAGE 使用 FreeLibrary。

初始化成功仍要求平台输入和 OpenGL 首帧完成。重复加载、旧 DLL 不含
新入口、平台不支持、依赖加载失败都会明确报告错误。

原生测试 `injector/tests/run-remote-thread.ps1` 在独立测试进程预加载
用户 JDK17 的旧运行库，验证远程加载、暂缓入口、CRT 绑定、mutex、
条件变量、线程计数、异常与 RAII、重复加载拒绝和 3 次卸载后重新加载。
正式 DLL 在没有可用 JVM 的测试进程中验证失败状态上报与自卸载。
反射加载的异常／卸载回归另外通过，真实 Minecraft 中的新模式待复测。
