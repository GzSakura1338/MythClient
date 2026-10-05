# Myth 项目交接文档

更新时间：2026-10-05

## 1. 项目定位

这是 Minecraft 26.2 Fabric 客户端 Mod，Java 版本为 25。源码统一位于 `src/dev/myth/`，资源位于 `src/resources/`，项目不使用 `src/main` 和 `src/test` 两套源码目录。

当前构建产物：`build/libs/myth-1.0.0.jar`。

## 2. 启动入口

主入口是 `src/dev/myth/Myth.java`，由它初始化以下全局系统：

- `EVENT_MANAGER`：事件注册和派发。
- `MODEL_MANAGER`：Combat、Misc、Player、Render、Client 模块生命周期。
- `COMMAND_MANAGER`：聊天命令和补全。
- `CONFIG_MANAGER`：JSON 配置保存与加载。
- `HUD_MANAGER`：Mio HUD 注册、布局、数据和编辑器。
- `UI_MANAGER`：ImGui ClickGUI、字体和 HUD 编辑器窗口。

停止游戏时，入口会先保存配置、关闭功能模块，再释放 UI 和 HUD 的渲染资源。

## 3. 目录说明

```text
src/dev/myth/
├── Myth.java
├── event/                 事件 API、事件类和 26.2 触发钩子
├── command/               .toggle、.bind、.config 等聊天命令
├── config/                配置读写和 latest 配置恢复
├── model/                 功能模块、分类和 ModelManager
├── utils/                 Keybind、移动、旋转及客户端工具
├── UI/
│   ├── ClickGui.java      Mio 风格 ClickGUI
│   ├── UiManager.java     UI 生命周期和渲染后端
│   ├── font/              自定义字体图集和原版字体切换
│   └── hud/               全部 HUD 模块和 HUD 编辑器
└── mixin/                 原版客户端事件和聊天 HUD 注入
```

## 4. HUD 状态

HUD 入口是 `src/dev/myth/UI/hud/HudManager.java`。当前已注册 19 个 Mio HUD：

`Armor`、`Chat`、`Crypto`、`Direction`、`Effects`、`EntityList`、`Graph`、`Inventory`、`Lag'O'Meter`、`Map`、`Metrics`、`ModuleList`、`Music`、`PlayerModel`、`Position`、`TextRadar`、`Totems`、`Watermark`、`Welcomer`。

HUD 设置、启用状态、锚点、自由位置和面板位置保存在配置的 `hud` 字段中。默认只有 `Watermark` 开启。HUD 编辑器支持：

- 六种锚点、自由拖动、中心吸附和方向键微调。
- 分类面板拖动、折叠和展开动画。
- 支持 `Size` 的 HUD 可使用右下角手柄拖动缩放，右键恢复为 `1.0`。
- Armor 按 Mio 逻辑固定在原版快捷栏附近，不参与普通 HUD 拖动。
- registry 多选设置支持搜索、分页和多选。
- Chat 通过 `MixinHudChat` 同步移动原版聊天和点击文本区域。

Map 只绘制模糊背景、面板底色、边框和中心的玩家圆点，不采集任何世界数据。Blur 默认开启，
与 ClickGUI 共用 `GaussianBlur` 缓存，但有自己的开关：`HudElement.wantsBlur()` 为真时，
`UiManager` 在普通游戏画面里也会准备模糊纹理，不再要求打开界面或 UI 的 Blur 选项；
Strength 和 FPS 仍沿用 Client → UI → Blur 的全局参数，因为缓存纹理只有一份。
Dot 颜色默认红色 `#FFFF0000`，支持 Rainbow / Sync，Radius 为 0.5～8 并随 Size 缩放。
原 NewChunks 数据源（区块分类、地牢标记和流体包监听）已按要求整体删除。

## 4.1 HUD 叠加与两遍渲染

HUD 之间允许自由叠加，模糊面板会把压在下面的 HUD 一起模糊，而不是盖掉它们。
实现方式是把一帧拆成两遍 ImGui 提交（`ImGuiRenderer.render(below, above, ...)`）：

1. 第一遍画 `wantsBlur()` 为假的全部 HUD（含编辑器的参考线、选中底色和元素本体），提交到后台缓冲。
2. `GaussianBlur.prepare` 此时才取样后台缓冲，于是模糊内容已经包含上面这些 HUD。
3. 第二遍画 `wantsBlur()` 为真的 HUD（Map）、编辑器面板和 ClickGUI，它们的模糊背景来自第 2 步。

关闭模糊时只提交一遍，两个回调依次执行，顺序不变。`HudManager.render(..., blurLayer)` 按
`wantsBlur()` 过滤元素，`HudEditor` 相应拆成 `renderBelow` / `renderAbove`。

副作用：`ImGui.newFrame()` 一帧会调用两次，`ImGui.isMouseClicked` 这种边沿状态只在第一遍成立。
因此 `ImGuiRenderer` 在第一遍把 5 个鼠标键的点击沿锁存下来，`ClickGui.click` 与 `HudEditor`
改用 `Myth.UI_MANAGER.isMouseClicked(button)`。新增 ImGui 边沿判定时必须走这个入口，
直接调用 `ImGui.isMouseClicked` 会在第二遍里永远为假。

## 5. 其他主要功能

`model/` 当前包含 KillAura / AutoCrystal（Combat）、FakePlayer（Misc）、Sprint / Scaffold（Player）、FullBright（Render）；
Notifications 属于 Client 分类，代码在 `command/Notifications.java`。功能模块的绑定支持 `Toggle`、`Hold`、`ReverseHold` 三种模式。

FakePlayer 把 Mio 的 `.fakeplayer` 命令改成模块：开启生成客户端副本、关闭移除，Record / Play 为开关。
26.2 的客户端伤害、吸收上限和爆炸包都变了，适配与未移植项见 [FakePlayer 移植记录](mio-fakeplayer-port.md)。

AutoCrystal 按爆炸伤害选落点与目标水晶，伤害模型在 `utils/CrystalDamage.java`。
Mio 那份反编译源码里暴露率是整数除法，恒为 0，所以原版「开了不放水晶」；本移植改用原版
`ServerExplosion.getSeenPercent`。BasePlace 会先垫黑曜石再放水晶；ESP 按要求用 ImGui 绘制，
见下节。静默换手、SpeedMine/AntiCheat 联动等未移植，清单见
[AutoCrystal 移植记录](mio-autocrystal-port.md)。**该模块尚未做运行时验证。**

## 5.1 世界 ESP

`Myth.WORLD_ESP`（`src/dev/myth/UI/WorldEsp.java`）是模块共用的渐隐世界盒子收集器，
对应 Mio 的 `AntiPhaseSearchHelper4`。不走 26.2 的 gizmo / 提交式渲染管线，
而是把 8 个角投影到屏幕后用 ImGui 画填充四边形与描边线。

投影矩阵取自 `gameRenderer.gameRenderState().levelRenderState.cameraRenderState` 的
`pos` / `viewRotationMatrix` / `projectionMatrix`，是这一帧实际渲染用的那组。
绘制发生在 `UiManager` 两遍提交的**第一遍**，所以模糊面板会把 ESP 一起糊进去。
新模块只要调用 `Myth.WORLD_ESP.submit(this, style, box)`，并在关闭时 `clear(this)`。

`command/` 中的通知逻辑负责在聊天栏显示模块开关提示；Notifications 本身属于 Client 分类，默认开启。

`utils/rotation/` 和 `utils/player/` 保留 Starain 的 Rotation、MoveFix 和相关移动工具，适配到 26.2 事件链。

## 6. 配置和运行

常用命令：

```powershell
.\gradlew.bat build --console=plain
.\gradlew.bat runClient
```

开发运行使用 DevAuth，本地配置通常位于：

```text
C:\Users\<用户名>\.devauth\config.toml
```

Mod 配置写入当前游戏目录：

```text
config/myth/latest.json
config/myth/<名称>.json
```

聊天命令默认前缀为 `.`，主要命令包括：

```text
.toggle <模块>
.bind <模块> <按键>
.config save <名称>
.config load <名称>
.config list
```

## 7. 验证结果

完整构建已通过：

```text
BUILD SUCCESSFUL
```

HUD 独立实例验证脚本位于 `build/all-hud-validation/`，执行：

```powershell
.\gradlew.bat -I build/all-hud-validation/validation.gradle runAllHudValidation --console=plain --no-configuration-cache
```

最近一次结果为 55 项 PASS，覆盖：

- 19 个 HUD 注册和默认启用状态。
- Position、Direction、Effects、EntityList、Metrics、TextRadar、Crypto。
- 真实物品、玩家模型、Totem、耐久和 Map 渲染。
- 原版字体、自定义字体和 MSAA。
- HUD 分类开合动画和完整收起状态。
- 配置保存、加载、设置和位置回环。
- 聊天点击区域、HUD 缩放手柄、右键重置。
- registry 搜索、分页、多选。
- Map 在普通游戏画面重新准备模糊背景（UI Blur 关闭时），以及默认红点和自定义颜色的实际像素。
- 普通游戏画面下 27 个背包格逐格取像素，27 件不同物品（含 3D 方块、盾牌和动画时钟）全部渲染；
  资源包重载、窗口缩放、GuiScale 切换和恢复之后再各验一次。
- Map 压在不透明背包面板上时，模糊均值从天空的 140 降到 29，证明模糊取到的是下层 HUD 而不是世界。
- 19 个 HUD 在同一屏同时启用时各自的着色像素数（`INFO` 行），用于发现整块不出图的组件。
- OpenGL 错误检查和正常游戏渲染。

验证输出保存在 `build/all-hud-validation/result.txt`，截图保存在 `build/all-hud-validation/`。

## 8. 已知限制

- `Crypto` 使用外部接口 `api.coinconvert.net`，网络不可用时显示 `UNKNOWN`，不会阻塞游戏线程。
- `Music` 使用 Windows Media Session PowerShell 脚本，仅在 Windows 上读取实际媒体播放信息；无活动媒体时编辑器显示预览文本。
- Map 开启 Blur 后，模糊纹理在普通游戏画面里也会按 Blur FPS 刷新，低配置机器会增加一次全屏两遍模糊的开销；关闭 Map 或其 Blur 即恢复原来的「只有界面打开时才模糊」。
- 模糊开启时一帧提交两遍 ImGui 绘制数据，开 MSAA 时也会多做一组 blit；关闭全部模糊即回到单遍。
- 模糊取自交换链后台缓冲，因此原版 HUD 和第一遍提交的 Myth HUD 都会被模糊进去；同一遍内的
  模糊组件之间不会互相模糊（Map 目前是唯一的模糊 HUD，所以不影响）。
- 模糊内容最多滞后一次 Blur FPS 间隔，快速变化的下层 HUD 在模糊里会晚一帧。
- 旧配置里遗留的 `pointer`、`pointerColor` 字段不再被读取，会原样保留在 `config/myth/*.json` 中，不影响加载。
- 玩家模型每帧生成 GUI 渲染状态，低配置机器上 HUD 编辑器可能降低帧率。
- 验证工程位于 `build/`，仅用于本地独立实例验证，不会打入发布 JAR。

## 9. 后续接手建议

1. 修改 HUD 时优先从 `HudElement`、`HudManager`、`HudEditor` 和 `LinesHudElement` 入手，避免绕过统一布局和配置系统。
2. 新增 HUD 先在 `HudManager.initialize()` 注册，再补充独立类和设置；不要新增语言 JSON。
3. 修改渲染资源时同步检查 `HudRenderAssets.NativeState`，避免污染 Minecraft 的 FBO、投影矩阵、视口或 scissor 状态。
4. 修改配置字段后重新执行独立 HUD 验证，确认旧配置仍能解码，且 `result.txt` 中没有失败项。
5. 发布前重新执行 `build`，确认 `build/libs/myth-1.0.0.jar` 和 sources JAR 均为最新时间戳。
