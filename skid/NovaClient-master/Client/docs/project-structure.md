# 项目结构与维护约定

公共组件与游戏目录直接并列在 `Client/src` 下。一个功能的实现、接口、设置和状态机集中在同一目录，子模式归到功能目录内部。

模块名称、配置键、注册顺序、JNI/native 方法签名与 DLL 输出路径不因本轮迁移而改变。

## 目录总览

```text
Client/
  CMakeLists.txt                # DLL、公共服务、链接和静态 TLS 检查
  include/                      # 反射加载器原有声明
  src/
    smtc/                       # 媒体会话、封面、歌词和播放器适配
    ui/
      clickgui/                 # 两种 GUI、控件、Panel、主题、字体
      hud/                      # 所有共用 HUD 的设置、绘制和动画
      backend/                  # 窗口、OpenGL/Vulkan/DX11、Blur
      overlay/                  # HUD 容器、拖动、位置保存、通用头像
      notifications/            # 通知队列与展示
      skin/ text/               # 图片资源、文字绘制
    config/
      settings/                 # 公共属性定义，原 setting/
      Config.* / ConfigManager.* / PropertyCodec.* / HostConfig.h
    bind/                       # 公共绑定、按键捕获与输入状态
    utils/                      # Color、Json、Keyboard；不读取游戏
    core/
      Hosts.*                   # 静态宿主选择
      Loader.cpp                # bootstrap 验证、宿主分派
      RuntimeServices.cpp       # 日志、熔断
      ReflectiveLoader.c / SelfUnload.* / Instance.h
    contracts/                  # GameHost、UiCatalog 等边界契约
    minecraft/
      CMakeLists.txt             # Minecraft 源码归属入口
      host/
        MinecraftHost.*         # JVM 启动、桥接和卸载
        presentation/           # 游戏数据、目录/配置适配、头像裁剪
        render/                 # 游戏帧接入与世界投影
      modules/
        CMakeLists.txt / Module.* / ModuleManager.* / Registry.*
        combat/                 # 每个战斗功能各自一个目录
        movement/ player/ misc/ render/
      compat/
        Profile.*               # Minecraft 版本能力选择
        mappings/               # 每个版本一份 .inc
        lunar189/ lunar1710/ minecraft263/ easecation/
      java/
        api/                    # 原 include/nova 的 Minecraft 原生接口
        Env.cpp / JniLookup.cpp / HookBridge.cpp / RandomName.cpp
        bytecode/               # 现有 C++ class 文件处理
        game/                   # Game*.cpp 与版本分发
      hooks/ events/            # 游戏 Hook 和事件总线
      utils/
        rotation/ inventory/    # Minecraft 旋转与背包工具
    cs2/
      host/                     # 原生宿主、公共 UI 与配置适配
      hooks/ sdk/ modules/      # DXGI 接入、引擎接口、功能扩展
    third_party/                # 第三方库，位置保持不变
  test/
    run-offline.ps1             # 不连接游戏的原生回归入口
    ui/ runtime/ render/        # 公共组件、启动、图形验证
    combat/ player/ misc/       # Minecraft 功能回归
  docs/
tools/check-client-layout.py    # 源码归属、引用和公共依赖检查
```

## 依赖与文件归属

- 游戏依赖公共组件，公共 `ui/smtc/config/bind/utils/contracts` 不得反向引用 `minecraft` 或 `cs2`。结构检查追踪传递 include，避免通过中间头文件重新耦合。
- Minecraft 模块位于 `src/minecraft/modules/<分类>/<功能>/`。AutoBlock 引擎由 KillAura 和独立 AutoBlock 共用；AntiKB 的 Prediction 子模式仍归 `antikb/prediction/`。
- HUD 的绘制、动画与布局位于 `ui/hud/<组件>/`。游戏模块负责筛选实体、统计、投影并提交数据，不复制 HUD 实现。
- 旋转和背包工具归 `minecraft/utils`；颜色、JSON、Win32 键盘等不依赖游戏的工具归公共 `utils`。
- `minecraft/java` 目录表示 JVM 互操作，生产实现是 C/C++；不新增 Java 源码或新编写的 Java 字节码。
- 配置文件仍使用既有目录与键名，不因源码移动而迁移用户文件。公共存储在 `config`，Minecraft 序列化适配在 `minecraft/host/presentation`。
- `core/Hosts.*` 是游戏路由点；公共 UI 不负责选择或启动游戏。CS2 profile 6 接入 DX11 骨架，见 [CS2 接入说明](../src/cs2/README.md)，实际游戏验证由用户完成。

## 构建与引用

根 CMake 通过 `add_subdirectory(src/minecraft)` 纳入 Minecraft。游戏目录自己登记宿主、Hook、事件、工具，并纳入 `compat/java/modules` 子清单；公共 `ui`、`smtc` 各维护清单。

每个生产编译单元只登记一次。头文件 glob 仅用于 Visual Studio 展示，不自动收集新实现。不要保留旧路径转发头，也不要手改 `build-*` 下的生成工程。

项目代码 include 从 `src` 起算：

```cpp
#include "ui/hud/nametags/NameTagView.h"
#include "config/settings/Properties.h"
#include "utils/Color.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/java/api/game_entity.h"
```

移动文件时同时更新 CMake、include、测试读取源码路径、文档链接和结构检查。第三方依赖与 `skid` 参考项目不参与路径重写。

## 验证

从仓库根目录执行：

```powershell
python tools/check-client-layout.py
& Client/test/run-offline.ps1
cmake --build Client/build-vs2026 --config Release
& Client/build-vs2026/preview/Release/NovaUiPreview.exe --smoke
```

结构检查覆盖旧目录残留、模块归属、生产编译单元唯一登记、内部引用、文档链接和公共组件依赖边界。独立 UI 预览不链接 Minecraft 实现。

构建产物仍是 `Client/build-vs2026/Release/NovaClientBase.dll`。离线回归不注入游戏；实际游戏注入和效果由用户测试。
