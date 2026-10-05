# Nova Client

版本扩展文档：[新增可注入 Minecraft 版本指南](docs/adding-client-version.md)。涵盖注入器版本选项、单文件 mapping、运行时与 Hook 适配、构建及验证；结构概览见 [多版本支持](docs/multi-version-support.md)。

Nova 的原生 C++ 客户端 DLL，包含功能模块、ClickGUI/HUD、多版本游戏适配、JNI/JVMTI 桥接和反射加载支持。目录归属和维护约定见 [项目结构](docs/project-structure.md)。

公共 UI、SMTC、配置、绑定、工具分别位于 `src/ui`、`src/smtc`、`src/config`、`src/bind`、`src/utils`。Minecraft 的模块、版本适配、C++ JNI、Hook、事件和游戏工具全部位于 `src/minecraft`；`src/cs2` 已接入 profile 6 的 DX11 原生宿主骨架，见 [CS2 接入说明](src/cs2/README.md)。项目功能不新增 Java 源码。

从仓库根目录构建：`cmake -S Client -B Client/build-vs2026 -A x64`，然后 `cmake --build Client/build-vs2026 --config Release`。已有构建目录继续使用其配置的 Visual Studio 工具链。产物为 `Client/build-vs2026/Release/NovaClientBase.dll`，构建末尾检查反射加载不支持的静态 TLS。

结构检查：`python tools/check-client-layout.py`。统一离线回归：`& Client/test/run-offline.ps1`。这些检查不注入游戏；实际注入及游戏内效果由用户测试。

`minecraft/modules`、`minecraft/compat`、`minecraft/java`、`ui`、`smtc` 各自的 `CMakeLists.txt` 管理所属编译单元，不再维护重复的 `sources.txt`。

运行时控制：注入后每个 DLL 实例都会启动独立的轻量控制线程。Minecraft 窗口处于前台时按 `DEL` 会打开当前实例的熔断状态并停止它的基础运行时。之后可以继续选择并注入同一个或不同的 DLL，注入次数不受限制；每个新 DLL 都有自己的状态。反射式加载器、远程线程和 DLL 映射流程保持不变。

## 输入工具

`src/utils/Keyboard.h/.cpp` 提供统一的 Win32 输入轮询：键盘和鼠标的按下、释放、边沿检测、鼠标坐标以及前台进程判断。

右 Shift 是 ClickGUI 的默认开关。`ui/clickgui` 提供界面，`ui/backend` 管理 OpenGL/Vulkan 渲染入口和共享输入。

`Settings → Client → Blur` 控制 HUD / ClickGUI 的局部背景模糊，支持自定义强度和背景刷新 FPS。设置、渲染接入及独立 GPU 验证见 [HUD 模糊](docs/hud-blur.md)。

## ClickGUI 控件库

`src/third_party/fatalwidgets` 是完整的本地副本，包含 FatalWidgets 控件层、Dear ImGui 核心、Win32/OpenGL3 后端、测试菜单、字体、许可证和原项目 CMake 文件。`skid` 目录不参与构建或运行。主 Client 工程只把控件层编译为 `NovaFatalWidgets` 静态库并链接到 `NovaClientBase`。
