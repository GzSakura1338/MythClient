# CS2 模块

按功能建立目录，每个模块的实现、设置与状态集中在自己的目录中。当前提供公共 UI/HUD、读取 CS2 玩家快照的 `render/esp2d`、控制原版雷达的 `render/radaresp` 和统一队友判断的 `misc/team`；2DESP 直接复用公共绘制，游戏数据保持在 CS2 层。Radar ESP 使用独立的原版雷达更新 Hook，不在 Present 中修改游戏状态。

- 引擎和实体访问通过 `../sdk`，不依赖 Minecraft。
- 派生 `Module`，在构造函数指定稳定名称、分类、页面、是否 HUD 和默认开关。
- 用 `addBool/addNumber/addMode/addSettings` 注册属性；自有属性与描述符名称需保持整个模块生命周期有效。
- 在 `RegisterBuiltinModules` 中用 `ModuleManager::Get().Create<T>()` 注册。Seal 后拒绝新增；同名模块也拒绝注册。
- FeatureCatalog、ArrayList、配置和绑定从模块管理器自动生成，不在 Presentation 中逐项手写。
- HUD 使用 `../../ui/hud`，不复制绘制实现。
- C++ 实现加入 `../CMakeLists.txt` 的显式源文件清单。
- 需要游戏线程的逻辑先定义对应 Hook 和生命周期，不在 Present 中直接执行。

## 生命周期与模式

`render/chams` 发布材质、颜色和 Through Walls 设置；`hooks/ChamsHook` 在原生场景绘制批次中筛选敌人并临时替换材质。`sdk/chams` 管理版本校验、实体身份及原生材质创建/释放。每次回调恢复原始字段，卸载排空后交由引擎删除资源。

`render/time` 提供 Day / Night 本地曝光预设，模块只发布设置；`hooks/TimeHook` 在经过版本和调用点校验的原生后处理计算中调整当前返回值。无持久实体写入；禁用、切图和卸载不需要恢复保存的地图指针。

`render/fovcontrol` 提供 FOV Control：模块发布 FOV / Preserve Zoom 设置，原生世界视野 Hook 在相机线程调整返回值。`sdk/camera` 验证本地相机与原生基础视野入口，不在 Present 改写相机属性。

`combat/recoilcontrol` 提供 Recoil Control：模块只发布四项设置，原生输入 Hook 执行反馈控制；枪械识别与有效后坐力求值在 `sdk/weapons`。不在模块的 Present 生命周期里调用游戏预测服务或写视角。

`combat/autofire` 提供独立 Auto Fire：Delay 和 Scoped Only 两项设置，默认 Toggle，队友由 Team 统一过滤。在同一次原生输入更新中用最终射击方向做单条射线，按 VData 和实际发射反馈处理自动/半自动/R8，合并原生按键和 subtick 边沿。模块不修改角度、不模拟系统鼠标；详见 [设计与实现](../../../docs/cs2-auto-fire-design.md)。

`combat/aimbot` 提供 Aimbot：默认 Hold；命中点和几何可见性由 `sdk/aim` 采集，`input/AimPipeline` 与 Recoil Control 合成一次最终角度。复用同一个 InputHook；模块更新只发布设置与 Team 策略。Only While Clicking 默认开启，输入回调检查物理左键；Show FOV 默认开启，调用公共 UI 绘制准星中心白色细线圆。Hold 状态不持久化，默认未绑定时不转头。

`Tick → onEnable/onDisable → onUpdate`，之后 `RenderOverlays → onRender`。禁用时需要继续退出动画或编辑预览的 HUD，覆写 `renderWhenDisabled()`。

要拆分模式实现时，派生 `Mode` 并在模块构造中用 `setModes` 一次性注册。每个模式可提供自己的 `settings()` 和回调。管理器只运行当前模式；运行中切换顺序为旧模式 `onDisable` → 新模式 `onEnable` → 当前模式更新。关闭模块先停止子模式，再停止模块。未启用时改变模式不运行回调；下次启用采用新模式。

普通枚举选项仍使用 `addMode`；需要独立实现与状态清理的功能使用 `setModes`。ClickGUI 的 Panel / Classic 是已接入的示例。

## 配置与输入

- 每个模块默认包含 Enabled 和 Bind，绑定复用公共 Toggle/Hold 行为。键盘、M1–M5、失焦释放和键盘自动重复过滤由 DX11 输入桥处理。
- 普通模块在 ClickGUI 中禁止快捷键激活，Hold 的释放仍生效；菜单自身允许在打开状态响应绑定。
- Hide in ArrayList 位于模块右键菜单，不混进参数齿轮。
- 模式按名字保存，所有子模式参数都保存，即使当前没有显示；旧 `features` 格式可读，新格式使用与 Minecraft 类似的 `modules` 对象。
- 停机先保存用户配置，再按注册逆序清理模块；不会把所有关闭状态覆盖到配置。
- ClickGUI 的打开状态不持久化，重新载入配置或注入不会自动弹出菜单。
