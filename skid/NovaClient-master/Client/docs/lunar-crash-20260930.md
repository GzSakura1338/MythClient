# Lunar 1.21.11，2026-09-30 01:59 崩溃

## 证据

- 启动器 `C:/Users/13366/.lunarclient/logs/launcher/main.log`：01:59:11，PID 54176，退出码 3221226505（0xC0000409）。没有 `hs_err_pid54176.log`。
- Windows Application Error：ucrtbase.dll + 0xA527E。转储 `C:/Users/13366/AppData/Local/CrashDumps/javaw.exe.54176.dmp`，异常线程 61112，异常参数 7（fatal application exit）。导出表定位到 `abort + 0x4E`；上层为 `terminate` 和 CRT 线程异常终止路径。
- 同线程栈中保存原始 C++ 异常记录：0xE06D7363，magic **0x01994000**，ThrowInfo 0x1AE975A30，**throw image base 为 0**。这不是普通 Java 异常，也不能仅根据外层状态码称为栈缓冲区溢出。
- 手动映射基址 0x1ACF20000；栈地址 RVA 0x2D533、0x2EC68、0x1EB33、0x1C74E 对应微软浏览器授权 `BrowserCode`、`microsoft::Login`、`accounts::Launch` 工作线程。与前部代码未变化的构建 MAP 对照，MAP 存在 `build-vs2026/diagnostics/lunar-20260930-0159/before-fix.map`，未将其冒充原崩溃 DLL 的完整符号文件。

## 原因

MSVC vcruntime 的 `_CxxThrowException` 用 `RtlPcToFileHeader(ThrowInfo)` 取得相对类型信息所属镜像。Nova 反射加载仅注册了 `.pdata`，没有进入 Windows 普通模块列表；该查询返回空，运行库把异常标记为 Pure/CLR magic（0x01994000），而不是 native C++ magic（0x19930520）。因此账号工作线程已经存在的 `catch(const std::string&)` / `catch(...)` 仍无法按正常方式捕获。

已通过独立进程复现：只加入 C++ throw/catch 的旧加载器测试返回 -529697949（0xE06D7363），原先的 SEH-only 测试不能覆盖这个问题。

## 修复

反射加载器只重定向 Nova 自身 IAT 的 `_CxxThrowException`。针对本镜像内的普通 native ThrowInfo，使用已知的镜像基址构造标准四参数 C++ 异常；其他 DLL 类型信息、重抛、Pure/WinRT 特殊路径继续调用原始运行库函数。不修改宿主运行库代码，不全局挂钩系统异常处理，不吞掉真正的未处理错误。

在 CRT 构造函数执行前完成 IAT 接入，保持原有 `.pdata` 注册及卸载清理。缺少异常入口时拒绝加载，不运行不完整状态。通用加载器改动不限定 Minecraft 版本。

构建现在同步输出 `NovaClientBase.map`，避免后续 native 地址仅能用旧 MAP 猜测。

## 验证和边界

`Client/test/loader/run_reflective_unwind.ps1` 覆盖反射加载/卸载循环、普通 LoadLibrary、CRT 构造期 C++ 异常、字符串及标准异常、RAII、嵌套重抛、exception_ptr、工作线程异常，以及原有 SEH/注册失败/畸形重定位路径。

修复后上述测试全部通过；`-ProductionDll` 进一步通过正式 DLL 的错误宿主拒绝、线程退出及自卸载检查。将本次 Lunar Zulu 21.0.10 的 `msvcp140.dll`、`vcruntime140.dll`、`vcruntime140_1.dll` 复制到独立测试目录后，同组测试再次通过。修复 DLL 构建时间 2026-09-30 02:19:00，28121088 字节，静态 TLS 检查通过。

这次修复的是已复现的 native 异常漏接。没有代替用户在 Lunar 中实测微软登录或注入。

另一次 00:55 的 PID 43448 有独立 `hs_err_pid43448.log`：JVM native 内存分配 1670256 字节失败，当时桌面窗口管理器也异常。它与本次账号线程异常有不同证据，不能混称同一个原因。
