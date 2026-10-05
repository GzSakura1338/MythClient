# WebView2 固定版浏览器内核

2026-09-27：改用微软发布的 x64 固定版 WebView2 `154.0.4258.37`，SDK `1.0.4191.47`。不再编译 Chromium，也不依赖电脑预装 Edge/Chrome。运行时通过明确的目录传给 `CreateCoreWebView2EnvironmentWithOptions`，缺失时报告错误，不回退到系统浏览器。

## 目录与分发

- 获取入口：`Client/src/chrome/tools/get-webview2.ps1`，固定官方 URL 与 SHA-256，并检查微软 Authenticode 签名。
- 开发依赖：`Client/src/chrome/vendor/webview2-sdk` 和 `vendor/webview2-fixed`。
- 宿主：`Client/src/chrome/host/WebViewHost.cpp`，单独的原生 C++ EXE。
- 构建产物：`Client/build-vs2026/browser-webview2/Release`，包含宿主和一份 `runtime`。
- 注入器打包：`tools/package-browser.ps1` 只收录这套运行时，排除 PDB、LIB、EXP；ZIP 使用 Optimal 压缩。
- 用户运行目录：`Nova/Browser/runtime/webview2-154.0.4258.37-v1` 下按压缩包哈希分版本。两个游戏分支复用一份内核。

官方 CAB 下载为 307,889,499 字节（约 294 MiB），解压运行时为 700,869,835 字节（约 668 MiB）。这是完整浏览器内核的大小，不承诺几十 MB。保留微软运行时文件、语言资源与许可证，避免通过任意删 DLL 破坏解码或更新；不分发 SDK、源码、下载缓存及旧 CEF。

`tools/cleanup-old-cef.ps1` 列出了废弃源码、CEF SDK、下载包和旧测试运行时的明确清理范围，可先传 `-WhatIf` 预览。本次工具自动审批拒绝递归删除 `vendor/source-build`，所以不能把仓库中的旧下载目录标记为已清理。

## 接入方式

游戏 DLL 继续使用现有 `BrowserService` / IPC，ClickGUI、标签栏、缩放、前台调度、OpenGL/DX11/Vulkan 的纹理上传接口不变。WebView2 CompositionController 输出到 Windows Composition，Windows Graphics Capture 将该宿主自己的视觉树捕获为 BGRA，写入现有共享帧邮箱。它不捕获桌面或其他应用。

宿主使用屏幕外工具窗口承载原生输入与合成目标，避免抢占游戏窗口。后台不发布画面；独立进程受原有 Job Object 管理，卸载会结束自己的浏览器进程。浏览器的正常沙箱保持启用，没有启用远程调试端口，也没有向网页暴露游戏内存接口。

## 验收

使用 `Client/test/chrome/kernel` 独立构建浏览器，不需要注入游戏。所有验收 profile 可通过 `NovaBrowserSmoke --data-root <项目内目录>` 存放在项目中。

- `--h264`：真实 MP4 视频播放、暂停、跳转、恢复与 AAC PCM 解码。
- `--cloud-h264`：本地 WebRTC 强制协商 H.264，实际解码视频帧并传递 data channel 数据。
- 默认测试：实际非黑画面、鼠标、键盘、中文提交、标签、调整大小、隐藏/恢复与关闭/重启。

H.264/AAC 和 H.264 WebRTC 已通过。B站账号/具体视频、云原神/云异环实际服务、指针锁定及相对鼠标操作仍需进一步验收，不能从本地解码测试推断全部服务已兼容。当前没有接入 DRM、文件下载或扩展安装。
