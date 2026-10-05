# CS2 原生宿主

已注册 profile 6（`cs2`），接入同一 `NovaClientBase.dll` 的原生宿主、DX11 和公共 UI。骨架经过独立原生程序验证，尚未由用户在实际 CS2 中注入验收。

## 结构

- `host/`：启动/停止、共享功能目录、配置适配与帧调用。
- `hooks/`：DXGI Present / ResizeBuffers 安装、回调计数和排空；只撤销自身 Hook。
- `sdk/`：Source 2 接口发现和 SDL 鼠标捕获；`sdk/players/` 管理版本校验、玩家偏移与只读快照。
- `modules/Module.*`：模块属性、键位、启停和子模式生命周期。
- `modules/ModuleManager.*`：注册、分类查找、统一更新/绘制、配置和停机管理。
- `modules/render/HudModules.cpp`：公共 HUD 和 ClickGUI 的薄适配；不复制 UI 绘制。
- `modules/render/esp2d/`：2DESP 设置、目标过滤与包围盒投影；直接调用公共 `ui/hud/esp2d` 绘制。
- `../ui/backend/Dx11Overlay.*`：公共 DX11 绘制、设备/窗口资源、输入队列。
- ClickGUI、HUD、主题、动画、SMTC、配置和绑定复用现有公共组件，不依赖 Minecraft。

参考 `gniesporek/internal-sdk` 的入口、接口、Hook、Renderer、功能分层，自行实现 Nova 适配。没有复制其整套 UI、游戏功能或 Steam Overlay 特征码。DX11 ImGui 后端来自与本项目相同的官方 v1.92.8。

## 启动与卸载

`Bootstrap(cs2) → core/Hosts → Cs2Host → SDK interfaces → DX11 hooks → shared UI`

1. 验证当前进程为 `cs2.exe`，等待 client/engine/schema/input/SDL/DX11 模块。
2. 通过模块导出的 `CreateInterface` 查找服务，不等待 JVM。
3. 用临时 DX11 交换链取得 DXGI 方法入口，安装 Present 与 ResizeBuffers Hook。
4. 首个成功绘制的帧之后才向注入器报告 Ready；无帧时超时失败。
5. ImGui 操作在呈现线程执行；窗口线程排队输入并处理 SDL 鼠标状态。
6. 卸载关闭菜单、恢复鼠标、保存配置、停止 SMTC、释放 GPU 资源、撤销 Hook，并检查渲染和窗口回调排空。暂时无法完成时继续等待并重试，不强制释放仍被使用的 DLL。清理完成后释放实例锁；最后再尝试回收停用镜像。

按键：`Insert` 打开/关闭 ClickGUI，`Esc` 关闭菜单，默认 `Delete` 请求卸载。在 CLIENT → Settings → Unload Key 中可修改熔断键，并随配置保存；Minecraft 与 CS2 共用该设置实现，各自保留配置。

熔断键由窗口线程直接接收，宿主线程另有前台物理按键检测，避免被普通绑定录入、UI 输入消费或渲染暂停阻断。只有录入熔断键自身时暂时抑制卸载，新键松开后再生效；它是按下一次触发的动作，不提供 Hold/Toggle 模式。日志按阶段记录卸载开始、等待清理以及实例锁释放。

配置：`%LOCALAPPDATA%/Nova/cs2/configs`；日志：`%LOCALAPPDATA%/Nova/cs2/nova-cs2.log`。与 Minecraft 配置分开。

## 范围与扩展

目前支持 DirectX 11。Client 设置中的 Blur、Blur Strength（1–6）、Blur FPS（5–120）已接入 DX11；Vulkan 宿主尚未实现。

DX11 在绘制 Nova 前捕获游戏背景，用与现有后端相同的降采样/重建滤波生成半分辨率缓存。只按 Blur FPS 更新缓存，面板、HUD 和弹窗仍按游戏帧率使用同一套圆角、裁剪和透明度绘制。缩放、切换设备或卸载时释放资源；计算着色器状态在过滤后恢复。

拉伸分辨率下，布局、投影和鼠标输入统一使用 Win32 客户区坐标，DX11 后端按实际后缓冲纹理尺寸设置每轴 `DisplayFramebufferScale`。因此 ESP、文字、护甲条、裁剪和 Blur 采样与游戏画面一起缩放，不额外套用 4:3 补偿，也不采用 Present 前可能属于后处理的 RS viewport。ResizeBuffers 或交换链/设备替换会重新获取纹理尺寸；仅改变窗口大小也会逐帧更新比例。零尺寸客户区暂停绘制，恢复后重新计算。

当前提供 ArrayList、Watermark、Notifications、MusicHUD/歌词、ClickGUI、2DESP、Radar ESP、Team、Recoil Control、Aimbot、FOV Control、Time 和 Chams。默认媒体服务关闭，启用 MusicHUD 后启动。2DESP 已接入真实玩家数据；Watermark 已接入本地玩家名、计分板 Ping、服务器 Tick 进度统计的 TPS、连接地址、坐标和水平速度（u/s），暂无数据的项自动隐藏。详见 [HUD 数据适配](sdk/hud/README.md)。

Aimbot 默认关闭，位于 Combat → General，默认 Hold 且未绑定按键。提供水平/垂直转速、Smooth、Aim FOV、Single/Switch、Head/Chest/Pelvis 优先部位、后坐力补偿和 Team 过滤；任一可见身体点入圈后优先瞄准所选可见部位，不要求该部位也在圈内；优先部位不可用时回退到圈内可见身体点。开火由用户或独立 Auto Fire 控制。与 Recoil Control 共用最终输入 Hook，几何射线验证遮挡；目前没有烟雾过滤。详见 [实施与验收边界](../../docs/cs2-aimbot-implementation.md)。

Auto Fire 的 Fire Policy 默认 Direct，不受新增的开镜/固定散布锥条件阻止；只有主动选择 Precision 才启用这些限制。此默认策略修复了 0.25° 精度门槛低于 AK/M4 静止散布锥而导致的不开火回归，旧配置中的精度开关不会自动启用 Precision。

FOV Control 默认关闭，位于 Render → General。FOV 可调 30–150，步进 0.1，默认 90；在开镜相机与武器覆盖之后修改本地视图，修复旧入口的设置被覆盖问题。Preserve Zoom 默认开启，开镜时保留倍率；关闭后包括开镜在内都使用设置值。Remove View Shake 默认开启，移除第一人称附加视角抖动并与输入补偿同步，不改真实弹道散布。详见 [FOV 实施说明](../../docs/cs2-fov-control.md)。

Time 默认关闭，位于 Render → General，齿轮中可选择 Day / Night 和各自亮度。通过原生后处理曝光模拟本地昼夜亮度，关闭后恢复原生计算；不改变服务器时间、天空内容或地图烘焙的太阳阴影。详见 [Time 实施说明](../../docs/cs2-time.md)。

Chams 默认关闭，位于 Render → General。移植 asphyxia 的 White / Illuminate 材质，支持可见/遮挡颜色和透明度，Through Walls 默认关闭，仅处理存活敌人。绘制后恢复原材质与颜色，卸载排空回调后释放材质引用。详见 [Chams 适配与验证范围](../../docs/cs2-chams.md)。

Recoil Control 默认关闭，位于 Combat → General，自动按物品编号识别枪械。提供 Horizontal、Vertical、Start Bullet、Smooth 四个参数，复用配置与绑定。压枪在原版最终输入视角提交点执行，不在 Present 改角度；详见 [实施与验收边界](../../docs/cs2-recoil-control-implementation.md)。

Auto Fire 默认关闭，位于 Combat → General，默认 Toggle。提供 Delay（0–500 ms，默认 0）和 Scoped Only（默认关），复用 Team 判断。用当前最终射击方向的第一处碰撞判断有效玩家，在原生输入样本中合并自动开火，保持手动左键；不要求启用 Aimbot。详见 [设计与实现](../../docs/cs2-auto-fire-design.md)。

Radar ESP 默认关闭，位于 Render → General。通过原版雷达玩家更新回调临时启用显示标志，显示双方玩家，保留原版地图样式，不受 Team 的 2DESP 过滤影响。标志在回调返回或异常展开时恢复；关闭、熔断时停用并排空专用 Hook。仅显示游戏已接收到有效数据的玩家，不生成服务器未发送的位置。版本或入口校验失败时仅停用该功能，详见 [雷达适配说明](sdk/radar/README.md)。

2DESP 默认关闭，位于 Render → General。Box 支持 Normal / CS / None，Outline 默认开启；血条支持 Health / Standard / Gradient / None，并提供护甲条、名称/文字阴影及玩家筛选。方框和进度条使用公共 `Esp2DView::Draw` 的 Compact 布局，参考 asphyxia 的对称描边和独立边距，保留 Nova 字体、主题与抗闪烁绘制；Minecraft 的原布局保持不变。玩家按深度从远到近绘制，长名称自动省略。每帧验证 controller → pawn 身份与存活状态；世界变换用于八角碰撞框，近平面继续裁剪，屏幕边缘不压缩框和血条。最大血量临时为 0 不再导致整个人不显示。详见[玩家读取与布局说明](sdk/players/README.md)。

Team 位于 Miscellaneous，默认关闭。开启时，2DESP 通过 `modules/misc/team/Team` 统一过滤与本地玩家同为 T 或同为 CT 的目标；关闭后显示双方。本地未分队或旁观状态不认定任何阵营为队友。SDK 仅提供每帧的原始队伍编号，模块负责过滤策略，不缓存队友名单。Team 支持绑定、配置和 ArrayList；旧 2DESP 的 Player Teammates 选项在缺少 Team 配置时迁移（false → Team 开启），显式 Team 配置优先。

新功能放入 `modules/<category>/<feature>/`，通过 ModuleManager 注册，自动生成共享 UI 目录和 ArrayList 数据。分类、图标和顺序对齐 Minecraft：Panel 显示 Combat、Movement、Player、Render、Miscellaneous，保留空分类；CLIENT 下单独显示 Settings。ClickGUI 位于 Render / General，其他 HUD 使用 HUD 分组。配置包括启用状态、Toggle/Hold 绑定、Hide in ArrayList、设置与子模式；兼容第一版骨架的 `features` 配置。ClickGUI 默认 Insert，可更换绑定；开关状态不写入持久配置。

子模式支持独立参数及 onEnable/onDisable/onUpdate/onRender，运行中切换先停止旧模式再启动新模式；所有模式参数都会保存，UI 只展开当前模式参数。详见 [模块扩展说明](modules/README.md)。引擎查询通过 `sdk` 封装并转换为公共 HUD 快照。当前更新入口是 Present，仅适合呈现工作；游戏线程相关操作先定义对应 Hook，不能直接在 Present 中执行。

玩家读取当前支持 Build 14181（2026-09-22 提取）。偏移和指纹集中在 `sdk/players/Offsets.h`，启动时校验 client.dll / engine2.dll 的 SHA256、镜像大小和引擎构建号；不匹配时禁用依赖该版本的 2DESP 和 Radar ESP，共享 UI 仍可使用。构建和运行均不依赖 `skid`；偏移来源与更新说明见 [玩家读取说明](sdk/players/README.md)。

## 验证

`NovaCs2Smoke` 在自己的 DX11 窗口实际安装生产 Hook，验证 Panel/Classic、HUD 输出像素、配置往返、GPU 目标恢复、跨线程输入、ResizeBuffers 和卸载。不会注入 CS2。

同时验证模块注册/启停/模式切换、配置迁移、鼠标与 Hold 绑定，并读回 GPU 像素验证 Blur 强度、透明度、缓存限频和资源重建。

`NovaCs2Smoke.exe --stretch-only` 使用独立隐藏窗口验证同尺寸、4:3/5:4 拉伸、等比例缩放、超宽窗口、客户区单独缩放及零尺寸恢复。GPU 像素检查覆盖投影、名称/血条/护甲条、右下角覆盖、裁剪和 Blur UV；原生窗口消息验证鼠标命中。该测试不注入或改动游戏窗口，实际独占全屏与驱动缩放效果仍由用户验收。

ESP 测试覆盖已存在实体、重复 Pawn、队友、死亡/休眠、失效指针、句柄复用、采集中删除、断线与近裁剪投影。`esp.bmp` 用模拟屏幕矩形验证实际共享 DX11 绘制；模拟数据只存在于测试程序。可用 `NovaCs2Smoke.exe --probe-esp <PID>` 对运行中的 CS2 做只读快照检查，不注入或调用游戏代码。2026-09-22 已读取到 Build 14181 对局中的玩家，实际注入显示仍待用户验证。

运行 `Client/build-vs2026/cs2-test/Release/NovaCs2Smoke.exe <输出目录>`，输出 Panel 截图及配置。宿主路由测试覆盖原 1–5 profile 和新增 6。DLL 构建后检查反射加载所要求的无静态 TLS。
