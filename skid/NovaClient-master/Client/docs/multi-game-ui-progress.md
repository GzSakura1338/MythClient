# 多游戏 UI / HUD 解耦实施进度

2026-09-22 补充：新增 [CS2 原生宿主](../src/cs2/README.md)，profile 6，复用共享 UI/HUD/SMTC，并增加共享 DX11 后端、模块/子模式管理和 DX11 Blur。2DESP 已读取 Build 14181 玩家快照并调用共享 ESP2D 视图；独立 DX11 Hook、UI、生命周期、GPU Blur 与 ESP 快照/投影测试通过，运行中游戏的只读玩家采集已验证。CS2 实际注入显示待用户验证。

完整目标仍以[设计计划](multi-game-ui-plan.md)为准。公共 UI/HUD 展示已经拆出，并能在没有 Minecraft/JVM 的原生预览程序中运行。多游戏 Runtime、配置分层和其他游戏注入尚未全部完成，不能把展示层独立化等同于整个计划验收。

## 已落地的边界

### 公共功能目录

- [UiCatalog.h](../src/contracts/UiCatalog.h) 定义游戏无关的分类、功能 ID、属性、HUD 标记和 ArrayList 隐藏选项。
- 两种 ClickGUI 通过 [FeatureTree](../src/ui/clickgui/FeatureTree.cpp) 消费目录，不遍历 Minecraft ModuleManager，不暴露 Module 类型。
- [Minecraft 目录适配器](../src/minecraft/host/presentation/UiCatalog.cpp) 保留现有模块顺序、名称、分类和设置引用。
- Panel 导航来自目录元数据，保留 Minecraft 原来的五类布局；其他类别名也可注册，超过五类时导航滚动。
- 目录在渲染前安装一次，校验 ID、分类和开关引用；拒绝运行中替换，防止现有属性行引用失效。
- ClickGUI 关闭事件通过宿主注册的回调同步开关；独立 UI 无须存在名为 ClickGUI 的游戏模块。

属性暂时仍沿用现有 Property 引用与原执行线程，并未完成跨线程命令队列。宿主必须让引用和标签存储活到 UI 消费者停止；此限制写在契约中。

### 已完整拆出展示的 HUD

| 组件 | 公共实现 | Minecraft 包装职责 |
| --- | --- | --- |
| MusicHUD / Lyrics | `ui/hud/musichud`、`ui/hud/lyrics` | 注册设置，传递启用/编辑状态 |
| ArrayList | `ui/hud/arraylist` | 采集功能名称、稳定 ID、后缀、启用/隐藏状态 |
| Notifications | `ui/notifications/NotificationHud.*` | 保留配置加载基线及模块状态变化采样，发送通知 |
| AutoBlockHUD | `ui/hud/autoblockhud` | 读取格挡统计、在原 tick 时点采集玩家皮肤，提交快照 |
| Compass | `ui/hud/compass` | 保留背包/世界条件，计算方向与游戏 GUI 比例 |
| Watermark | `ui/hud/watermark` | 玩家/服务器信息、速度、ping 和 TPS 采样 |
| TargetHUD | `ui/hud/targethud` | 目标筛选、生命值修正、记分板锁、原 tick 快照发布 |
| NameTags | `ui/hud/nametags` | 实体过滤、可见性、位置投影、Classic 游戏格式码解析 |
| ESP2D | `ui/hud/esp2d` | 实体筛选、包围盒投影、生命值与名称采集 |
| BedPlates | `ui/hud/bedplates` | 床扫描、排序、范围判断、材质数据与位置投影 |
| ItemESP | `ui/hud/itemesp`、`ui/hud/world` | 掉落物读取、过滤、包围盒与文字锚点投影 |
| ChestESP / Backtrack | `ui/hud/world` | 箱体投影；Backtrack 保留原近裁剪处理并提交屏幕几何 |
| Breaker | `ui/hud/breaker` | 挖掘状态、方块投影、底部占用区域计算 |
| BlockIn | `ui/hud/blockin` | 计数、工作状态与时间戳 |

绘制、动画和背景均在公共实现；旧模块只包装或采集数据。原模块名、设置键、Overlay ID 和初始设置保留。ArrayList 的显示设置与平面配置行分别保留，维持原有重复颜色键的顺序。

公共 `Portrait` 支持图片资源、UV 图层、采样方式；Minecraft 的脸部/帽子裁剪规则放在 `minecraft/host/presentation/PlayerPortrait.h`。TargetHUD 的绘制、动画和设置在公共层，卡片持有输入快照的副本。

Minecraft 相机、bobbing、近裁剪面和投影实现已搬到 `minecraft/host/render/WorldProjection.*`，使用 `nova::minecraft::projection` 命名空间。实体 HUD 从游戏侧取得屏幕坐标，公共 UI 不读取相机或实体。

### 配置、运行时与渲染入口

- 公共 `config/Config.cpp` 负责 Client 设置、窗口位置与文件读写；`PropertyCodec.*` 保留重复属性键、旧子菜单数据兼容和序列化顺序。
- `config/HostConfig.h` 安装宿主目录和同步保存/读取回调；Minecraft 模块配置、Aim 旧配置迁移及版本目录选择位于 `minecraft/host/presentation/Config.*`。
- 原有 `.nova/configs` 目录及 profile 子目录保持兼容。保存改为同目录唯一临时文件、刷新后原子替换，避免留下截断配置。尚未实现公共偏好与游戏覆盖层合并。
- `core/Loader.cpp` 只验证 bootstrap ABI、分派 `GameHost`、报告结果并提供进程实例/卸载事件服务；JVM 等待、HeyPixel 启动、事件桥和 Minecraft 卸载在 `minecraft/host/MinecraftHost.cpp`。
- `core/RuntimeServices.cpp` 提供日志与熔断标记，公共渲染不再通过 JNI 头引用这些服务。卸载事件清除与请求使用 SRWLOCK，宿主清除后才关闭句柄。
- 现有五个 profile ID 保持不变，包括后续加入的 EaseCation。未知 profile 报错后镜像不启动宿主；尚未统一失败镜像的通用回收流程。
- `VulkanOverlay` 使用宿主提供的 frame/rendererLost/submitted 回调，不再直接引用 Minecraft OpenGL 入口。
- 原混合 `GlHook.*` 移至 `minecraft/host/render/OverlayHost.*`。其中 OpenGL 状态保护、窗口/输入、帧调度仍有待进一步拆分，不能宣称已完成全部后端生命周期抽象。

### 本轮界面问题

- 属性组件以自身地址作为稳定 ImGui ID，修复 Sneaking/Timing 等没有直接 Property 的设置组互相冲突。
- Panel 两列模块列表取消自绘竖向滚动条，保留滚轮滚动；长搜索结果已做交互回归。

### 编译边界

- `NovaUI` 静态库编译公共 UI/HUD，不配置 JNI/game API include 根目录；`NovaMedia` 编译 SMTC，`NovaRender` 编译独立 Vulkan 后端。
- `tools/check-client-layout.py` 检查全部 ui/contracts/config/smtc/bind/utils 的传递 include，没有公共层豁免项，拒绝引入 minecraft 或 cs2，也检查旧根目录是否残留源码。
- `test/ui/preview` 的 `NovaUiPreview` 链接真实 `NovaUI`、`NovaMedia` 和公共配置/绑定实现，不链接 Minecraft 实现或假 Module 服务。展示全部共享视图、两种 ClickGUI、明暗主题和配置保存/加载。
- 预览产物：`build-vs2026/preview/Release/NovaUiPreview.exe`。Insert 开关菜单，F2 切换 Panel/Classic，F3 切换主题；`--smoke` 用隐藏窗口自动检查。配置独立存于 `%TEMP%/NovaUiPreview`，不会覆盖游戏配置。
- 预览先绘制测试背景，再采样 Blur，最后提交 UI，避免递归采样 HUD；音乐视图使用现有占位预览，不启动真实媒体播放控制。

## 已执行验证

以下测试使用原生本地测试窗口或数据，未注入游戏：

| 验证入口 | 本轮覆盖 |
| --- | --- |
| `test/ui/catalog/run.cmd` | 无 Module/JNI 的目录安装、无效数据拒绝、属性绑定、HUD 分组、会话存储稳定性 |
| `test/smtc/run_hud.cmd` | 直接链接公共 MusicHud，移除 Module/JNI 测试依赖；歌曲/歌词预览、展开收回、缩放、控制及来源菜单 |
| `test/ui/hud/run.cmd` | 全部公共 HUD；行退出动画、无缝隙、缩放、头像、通知过滤、配置行、TargetHUD 时间动画及世界几何 |
| `test/render/panel/run.ps1` | 真实 Panel 控件与测试目录：参数、子窗口、颜色、绑定、导航、搜索、缩放、上下文重建；无属性设置组 ID、隐藏滚动条与滚轮滚动 |
| `test/vulkan/run.cmd` | Vulkan 模糊、字体/皮肤/封面、替换、resize、卸载与重建 |
| `test/render/surface/run.ps1` | OpenGL compatibility/core 的窗口/上下文变更、字体/皮肤/封面/Blur 恢复 |
| `test/runtime/run.cmd` | 生产 Loader/RuntimeServices/Hosts，无 JVM 假宿主：ABI 拒绝、五个 profile 路由、Ready/Failed 一次性报告、旧入口上下文、实例互斥和卸载事件竞争 |
| `test/combat/aimassist/run.ps1` | 真实 PropertyCodec 的旧配置迁移和子菜单保存/加载 |
| NameTags / ESP2D / ItemESP 原有测试 | Classic 格式与字体、实体屏幕几何和标签兼容 |
| `NovaUiPreview.exe --smoke` | 150 帧真实 OpenGL 独立预览，Panel/Classic、明暗主题、全部 HUD、配置往返、GL 错误检查 |
| `python tools/check-client-layout.py` | 源码归属、引用、文档链接、公共 UI 传递依赖边界 |
| Release DLL 构建 | `NovaUI.lib` 与 `NovaClientBase.dll` 链接；静态 TLS 检查 |

主要日志在 `build-vs2026/`：`ui-decouple-build.log`、`ui-catalog-test.log`、`ui-catalog-panel-test.log`、`ui-decouple-music-test.log`、`ui-common-hud-test.log`、`ui-decouple-vulkan-test.log`、`ui-decouple-surface-test.log`、`ui-runtime-test.log`、`ui-preview-smoke.log`、`ui-config-migration-test.log`。

原生预览使用公共目录及真实配置实现，运行时测试只替换游戏入口。上述验证不包含游戏内注入、真实服务器或完整游戏输入链路；游戏内视觉、输入和注入由用户验证，本轮没有自动注入。

## 下一轮需要继续的完整范围

### 2026-09-22 平级目录迁移验证

- `src` 当前只保留 bind、config、contracts、core、cs2、minecraft、smtc、third_party、ui、utils；旧 games/module/compat/java/hook/eventbus/util/setting 根目录已移除。
- Minecraft 由自己的 `CMakeLists.txt` 汇总，移动路径同时更新引用、原生测试及文档。配置文件、profile ID、mapping 内容和模块行为不因路径迁移改变。
- 完整 Release DLL 构建与静态 TLS 检查通过，产物仍为 `build-vs2026/Release/NovaClientBase.dll`。
- 30 个既有离线套件均已完成。首次运行暴露 Backtrack 旧投影替身和 ThrowableAura 旧发包替身未同步的问题，更新测试夹具后从失败套件继续，未修改游戏逻辑。记录见 `flat-layout-offline.log`、`flat-layout-offline-resume.log`、`flat-layout-offline-rest.log`。
- 公共目录、共享 HUD、Panel、NameTags 外观配置、SMTC 和无 Minecraft/JVM 的完整预览通过；五个 profile 的宿主分派测试通过。
- 结构检查通过：179 个原生编译单元唯一登记，1488 处内部引用可解析；公共依赖边界与文档链接有效。没有执行游戏注入。

### 尚未完成的范围

1. 建立 HUD 快照的会话/帧/视口代次契约、过期命令拒绝与 UI 命令路由；当前同步属性引用不能当作任意跨线程接口。
2. 实现公共偏好和游戏配置覆盖层、版本化迁移及多实例合并，当前只完成存储与宿主序列化边界。
3. 继续抽离 OpenGL 帧调度、窗口/输入和统一停止/drain 生命周期。Module 的 ScreenOpen 判断仍在 Minecraft 模块基类中。
4. 游戏代码已归并到 `src/minecraft`：modules、compat、java/api、hooks、events、utils 和 host；公共工具改为 `src/utils`、属性改为 `src/config/settings`，宿主分派在 `src/core/Hosts.*`。这完成目录归属，不等于公共帧调度全部抽离。
5. 首个其他游戏确定为 CS2，`src/cs2` 已与 Minecraft 平级预留。接下来实现 DirectX 11 接入、宿主和 GUI/CLI 目标选择，并维持单文件发布。当前注册的宿主仍只有 Minecraft。

不要将本轮已完成的组件集合当作目标范围。完整计划尚未完成，活动目标应保持进行中。
