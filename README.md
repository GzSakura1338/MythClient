# Myth

Minecraft 26.2 的 Fabric Mod 基础项目，使用 Java 25 或更新版本。

源码统一放在 `src/dev/myth/`，资源放在 `src/resources/`，不使用
`src/main`、`src/test` 或独立的客户端源码目录。

```text
src/
├── dev/myth/
│   ├── Myth.java        # 客户端入口
│   ├── utils/           # Keybind、ClientUtils、TimerUtils、Starain Rotation/MoveFix
│   ├── event/           # api、注解、23 个事件和渲染事件桥接
│   ├── command/         # 聊天命令、别名和补全
│   ├── config/          # JSON 配置保存与加载
│   ├── UI/              # Mio ClickGUI、ImGui 后端、字体库与 HUD
│   │   └── hud/         # HUD 注册表、布局、编辑器与 ModuleList
│   ├── mixin/           # 26.2 的事件触发钩子
│   └── model/           # Model、ModelManager、Category
│       ├── combat/      # KillAura
│       ├── misc/
│       ├── player/      # Sprint、Scaffold
│       ├── render/      # FullBright、NoRender
│       └── client/
└── resources/
    ├── fabric.mod.json
    └── myth.mixins.json
```

在当前目录执行：

```powershell
.\gradlew.bat build
.\gradlew.bat runClient
```

构建产物位于 `build/libs/myth-1.0.0.jar`。
Minecraft 26.2 使用官方未混淆名称，因此不需要额外配置 Yarn mappings。
DevAuth 1.2.2 使用 `localRuntime` 加载，仅用于开发运行，不打入发布 JAR。
`runClient` 已设置 `devauth.enabled=true` 和 `devauth.account=main`，选择配置中的 `[accounts.main]`；
首次登录按控制台给出的微软授权链接操作，
配置默认位于 Windows 用户目录的 `.devauth/config.toml`。

## 基础框架

`Myth.MODEL_MANAGER` 管理功能的注册、分类查询、启停、按键切换和客户端 tick。
当前注册 KillAura / Velocity（Combat）、FakePlayer（Misc）、Sprint / Scaffold / InvManager / ChestStealer（Player）、FullBright / Animations / Shader / TimeChanger / NoRender（Render）和 Notifications（Client），均默认未绑定。
原版选项页面的 FOV 滑条沿用 Mio 的上限 150（原版为 110），最小值 30、默认值 70；数值保存在原版 `options.txt`。
KillAura、Sprint、Scaffold、FullBright 和 NoRender 默认关闭；Notifications 默认开启，代码位于 `command/Notifications.java`。
KillAura 的 CPS Mode → 1.9+ 接入 Nova 等待暴击：开启 Wait Critical，右键展开 Interrupt Sprint 和 Critical Sound；三个选项默认关闭。
Notifications 的 Modules 控制聊天栏开关通知：开启显示绿色 `[+] Module`，关闭显示红色 `[-] Module`，
括号沿用 Mio 的较深颜色。GUI、按键和 toggle 命令共用通知入口；重复设置相同状态不会重复通知，
加载配置、回滚和退出时静默恢复状态。通知模块及 Modules 设置随既有配置保存。
Sprint 沿用 Starain 的 PRE Motion 最高优先级监听：持续按住原版冲刺键并关闭 ToggleSprint，关闭模块时释放冲刺键。
Scaffold 沿用 Starain 的 Normal / Snap / Telly、SafeWalk、Rise / Heypixel 转头和 Normal / Strict 射线判断，自动选择热栏方块，关闭时恢复槽位；按要求不包含 ESP、卡空跳帧和秒放方块自救。
源码适配和验证见 [Scaffold 移植记录](docs/starain-scaffold-port.md)。Scaffold 开启时暂停 KillAura。
InvManager 和 ChestStealer 移植自 Starain 的同名模块，共用 `utils/inventory/InventoryUtils`。
InvManager 在 `MotionEvent.PRE` 把最好的剑/方块/镐/斧/弓/末影珍珠/水桶/火球/蛋雪球换到指定热栏位，
按 Offhand Items 维持副手，Auto Armor 换装，Throw Items 丢掉不要的东西；
每个动作共用一个随机延迟，所以一个 tick 只点一次。ChestStealer 只认箱子/大箱子（可选末影箱），
每次搬一格最该拿的，搬空后按 Close Delay 关闭。
26.2 删掉了 `SwordItem` / `ArmorItem` / `ArmorMaterials` 这些类，所以分类改走 `ItemTags` 与
`Equippable` 组件，护甲和武器评分按属性修饰符重新推导，差异见
[InvManager 与 ChestStealer 移植记录](docs/starain-inventory-port.md)。
FakePlayer 沿用 Mio `.fakeplayer` 的客户端副本：开启在本地玩家位置生成一个复制背包的假人，
弹图腾不死、每 80 tick 补药水与 16 点吸收、副手常驻图腾；Name 改名即重新生成，
Record / Play 录制并循环回放自己的移动。打它会在本地结算伤害，攻击包不会发给服务器。
源码适配和未移植项见 [FakePlayer 移植记录](docs/mio-fakeplayer-port.md)。
Velocity 移植自 `skid/OpenOnxy-main` 的 `module/combat/VelocityModule`（1.8.9 的混淆反编译）。
只有 Lag 一种模式：挨打之后把自身的动量包压住，连它后面的包一起入队取消，
压够 Delay ticks 或落地、入水、减伤成功就按顺序重放；Keep Alive 和登录/重生包不压。
Reduce knockback 在击退落地后开一个 Reduce window，窗口内若准星 3 格内有人且正在疾跑，
就发裸 `ServerboundAttackPacket` 反打一次，水平动量 ×0.6 并取消疾跑。
Targets / FOV 限定目标，Chance 控制触发概率，Pause while backpedaling 和 Only while moving
按移动输入设闸。混淆名的语义考据、26.2 包名对照和重放的线程处理见
[Velocity 移植记录](docs/openonyx-velocity-port.md)。
Animations 移植自 `skid/Starain-1.21.11-master` 的 `module/impl/render/Animations`，**纯视觉、不发任何包**：
右键持剑时原版本来就会发 use 包，ViaVersion 把它翻译成 1.8 的格挡，服务端那半不用管；
缺的只是姿势，因为 26.2 的剑没有 BLOCK 使用动画。
Block Mode 有 Vanilla / Smooth / 1.7 / Push / Swing / Swang / Swong / Swaing / Punch / Sile / None 十一档，
另有 Position X/Y/Z、Swing Speed、Custom Equipped Progress 和 Hide Offhand。
由 `MixinItemInHandRenderer` 接管 26.2 改名后的 `submitArmWithItem`（源项目是 `renderArmWithItem`）。
Swing Speed 源项目靠反射 `LivingEntity.swingDuration`，26.2 没有这个字段（时长改由
`getCurrentSwingDuration()` 从物品的挥手动画算），所以改成在 `MixinLivingEntity` 里缩放那个返回值。
FullBright 保留 Gamma/NightVision 模式，Gamma 的 Brightness 为 1～15、默认 15，每个玩家 tick 增加 0.1。
26.2 通过 `LightmapRenderStateExtractor` 替换光照的 Gamma 读取，不修改游戏保存的亮度选项；关闭后恢复原版光照。
NightVision 每 tick 添加 1337 tick 的客户端夜视，切回 Gamma 或关闭模块时移除，与 Starain 一致。
Shader 移植自 Mio 的 `module/render/Shader`，是把 ESP 画成**后处理**而不是光影：
先拿到一张只有目标实体的白色剪影，再用 Solid / Rainbow / Gradient / Bloom 四个片元着色器
把它变成填充 + 描边，另有 Dots / Grid 装饰层。
**取剪影的方式整个换了**：Mio 自己换 `worldRenderer` 的 framebuffer 重渲染实体，
26.2 的帧图下这条路不存在；改成给目标实体置 `EntityRenderState.outlineColor`，
由原版渲染进它自己的 `entityOutlineTarget`，再屏蔽掉原版的描边后处理链。
GL pass 在 `LevelRenderer.render` 的 TAIL 直接混合到主画面（不经 ImGui），和 Mio 一致。
设置默认值取自 Mio 的 `nick/SettingValues` 表；Image / Hands / NoCluster 淡出未移植，
原因见 [Shader 移植记录](docs/mio-shader-port.md)。
TimeChanger 移植自 Starain 的同名模块：World Time（0～24000，默认 8000）把世界时间钉住，
并取消 `ClientboundSetTimePacket` 以免服务端定期回正导致闪烁，纯客户端。
**26.2 已经没有 `Level.setDayTime`**——时间换成了 `WorldClock` 注册表 + `ClockManager`，
维度的 timeline 取 `clockManager.getTotalTicks(clock)`，天空颜色、光照等级和日月位置都由它派生，
所以改的是 `ClientClockManager.handleUpdates` 里那个 clock 的 tick 数。
NoRender 沿用 Mio 的 46 个开关，分 Self / UI / World / Entities 四个折叠分区（这四个在 Mio 里也只是分区，本身无行为）；
名称、默认值、范围、单位和父子可见性全部取自 Mio 的 `nick/Setting*` 表，`isDrawn()` 为 false 所以不进 ModuleList。
26.2 把渲染拆成状态提取与提交，实体和方块实体的剔除因此落在提取阶段
（`LevelExtractor.isEntityVisible`、`BlockEntityRenderDispatcher.tryExtractRenderState`）。
Hands / Armor 的 Opacity 只有 0 与非 0 两种效果：26.2 的提交式管线没有全局 shader color，
而 Mio 默认值就是 0，所以开箱行为一致。NoCluster 与 IgnoreESP 未移植，
原因和完整落点见 [NoRender 移植记录](docs/mio-norender-port.md)。

后续功能继承 `Model`，指定唯一 ID、名称、分类和可选默认按键，然后在
`Myth.onInitializeClient()` 中调用 `MODEL_MANAGER.register(...)`。默认按键为空，
注册的按键可在游戏的控制设置中修改，当前不包含语言资源。
模块实现 `onEnable()`、`onDisable()`、`onTick(Minecraft)` 即可接入生命周期。
除 `CLIENT` 分类外，默认只在世界与玩家存在时调用 tick，可覆盖 `requiresWorld()`。
Toggle 在按下时切换，Hold 在按住时启用，ReverseHold 在松开时启用。
菜单中丢弃 Toggle 事件；已启用的 Hold 可在松开时关闭，ReverseHold 可在松开时恢复。
Hold/ReverseHold 在每个渲染帧直接读取 GLFW 键鼠状态，长按不会连续切换。

所有初始化仍由 `Myth` 承担，未增加 `MythClient` 类；该项目仅在客户端加载。

## Rotation 与 MoveFix

`utils/rotation/` 保留 Starain 的 `Rotation`、`RotationManager`、`RotationUtils` 和 `RayCastUtil`；
`utils/player/MoveUtils.java` 保留其移动工具及 `fixMovement`。`RotationManager` 在启动时注册一次，
不作为功能模块展示。调用 `RotationManager.setRotations(rotation, speed)` 后按 Starain 的事件链自动修正移动、
跳跃、射线、物品使用、运动包及模型旋转，PRE Motion 后重置本 tick 的目标和平滑状态。

保留源代码的 Lombok 注解，使用相同的 1.18.46 版本，仅参与编译。
逐行对照记录见 [Starain 移植核对](docs/starain-rotation-port.md)，转头连续性修复见 [Nova yaw 修复](docs/nova-yaw-continuity.md)。
`.\scripts\verify-starain-rotation.ps1` 校验 5 个仍保持原样的工具类，以及 2 个已核对的 Nova 修订版。

## 事件系统

与 `skid/Starain-1.20.1-main` 的 `event/api`、`event/impl` 对齐：
通过 `Myth.EVENT_MANAGER.register(object)` 注册 `@EventTarget` 方法，
`@EventPriority` 的数值越小越先执行，默认优先级为 `Priority.MEDIUM`。
`call(event)` 返回同一个事件实例，仅派发给参数类型与事件实际类型完全一致的监听方法。
取消事件后仍会调用后续监听者，由触发钩子读取最终取消状态决定是否继续原版行为。
监听者注册时建立排序快照，派发期间的注册和注销从下一次派发生效；重复注册同一方法不会重复调用。

`Model` 开启时注册事件监听，关闭或开启失败时注销监听。
`ModelManager` 通过 `GameTickEvent.POST` 处理按键和兼容的 `onTick` 回调。
事件类显式提供构造函数和 getter/setter，不依赖 Lombok。

| 事件 | 26.2 触发位置 |
| --- | --- |
| `GameTickEvent` | `Minecraft.tick` 的 PRE/POST |
| `ClickEvent` | `Minecraft.handleKeybinds` 的攻击/使用处理前，可取消 |
| `KeyboardEvent` | 主窗口 `KeyboardHandler.keyPress`，携带当前界面 |
| `MouseButtonEvent` | 主窗口 `MouseHandler.onButton`，携带当前界面 |
| `ChatEvent` | `ClientPacketListener.sendChat`，可改写或取消发送 |
| `PacketEvent` | 客户端 `Connection` 的 INCOMING/OUTGOING，可替换或取消包 |
| `PlayerTickEvent` | 本地玩家调用父类 tick 前，可取消 |
| `RespawnEvent` | 本地玩家首次有效 tick，包括进入世界与重生 |
| `MotionEvent` | `LocalPlayer.sendPosition` 的 PRE/POST；PRE 可改写位置、视角、落地状态或取消发送 |
| `SlowdownEvent` | 物品使用引起的移动减速和冲刺限制，可取消 |
| `MoveInputEvent` | `KeyboardInput.tick` 后，可改写移动、跳跃、潜行及潜行减速倍率 |
| `RayTraceEvent` | 本地玩家 `Entity.getViewVector`，可改写射线视角 |
| `StuckInBlockEvent` | 本地玩家 `Entity.makeStuckInBlock`，可改写或取消阻滞倍率 |
| `StrafeEvent` | 本地玩家 `Entity.moveRelative`，可改写移动 yaw |
| `JumpEvent` | 本地玩家 `LivingEntity.jumpFromGround`，可改写冲刺跳跃 yaw |
| `FallFlyingEvent` | 本地玩家 `LivingEntity.updateFallFlyingMovement`，可改写鞘翅 pitch |
| `RotationAnimationEvent` | 本地玩家身体转向及实体渲染状态提取，可改写插值视角 |
| `AttackYawEvent` | 本地玩家攻击击退/横扫方法，可改写攻击 yaw |
| `AttackSlowdownEvent` | 本地玩家攻击击退后的减速和停止冲刺，可取消 |
| `UseItemRayTraceEvent` | 本地玩家 `Item.getPlayerPOVHitResult`，可改写物品射线视角 |
| `PositionItemEvent` | `MultiPlayerGameMode.startPrediction` 发送物品使用包前，可替换或取消 |
| `RenderSkiaEvent` | `Hud.extractRenderState` 后，携带 `GuiGraphicsExtractor` 与 partial tick |
| `RenderWorldEvent` | Fabric `LevelRenderEvents.COLLECT_SUBMITS`，携带实际 `PoseStack` 与渲染上下文 |

26.2 已将绘制拆分为状态提取与提交：`RenderSkiaEvent` 保留参考项目的名称，
参数适配为 `GuiGraphicsExtractor`，此处只提供事件，不附带 Skia 渲染后端。
世界渲染通过 `RenderWorldEvent.getContext().submitNodeCollector()` 提交绘制。
`PositionItemEvent` 对应携带视角的 `ServerboundUseItemPacket`；26.2 已不再发送 1.20.1 的前置位置包。
网络 INCOMING 事件在 Netty 接收线程触发，监听者访问游戏状态时须安排到客户端线程。

26.2 分析源码和官方客户端 JAR 位于 `skid/minecraft-26.2/`，仅作参考，不参与编译。

## 聊天命令与配置

聊天命令默认使用 `.` 前缀，可在 UI 的 Prefix 中修改，支持 Tab 补全，大小写不敏感。以前缀开头的消息会被
`ChatEvent` 拦截并留在本地，未知命令会显示用法，不发送到服务器。
反馈直接使用文本，不包含语言资源。

| 命令 | 别名 | 用途 |
| --- | --- | --- |
| `.toggle <模块>` | `.t` | 切换模块并保存配置 |
| `.bind <模块> <按键>` | `.b` | 设置键盘或鼠标绑定并保存配置 |
| `.bind <模块> None` | `.b` | 解除绑定 |
| `.config save <名称>` | `.cfg save` | 保存并选中配置 |
| `.config load <名称>` | `.cfg load` | 加载并选中已有配置 |
| `.config list` | `.cfg list` | 查看当前与可用配置 |

模块参数可使用模块 ID 或名称；补全使用 ID。按键支持 `A`～`Z`、`0`～`9`、
`F1`～`F25`、`RShift`、`LCtrl`、`Space` 等，以及 `MouseLeft`、`MouseRight`、
`MouseMiddle`、`Mouse0`～`Mouse7`（GLFW 编号从 0 开始）。
例如 `.toggle sprint`、`.toggle fullbright`、`.bind sprint F8`；GUI 的 Bind 也支持 Toggle/Hold/ReverseHold。
扩展命令继承 `Command`，调用 `Myth.COMMAND_MANAGER.register(...)`。

配置存放在当前游戏目录的 `config/myth/<名称>.json`，启动默认加载 `latest.json`。
先注册模块并初始化按键，在原版 Options 加载完成后恢复配置，避免 `options.txt` 覆盖配置中的绑定；添加模块应在 `MODEL_MANAGER.initialize()` 之前注册。
配置保存已注册模块的 `enabled` 和键鼠绑定字符串，保留未识别模块和扩展字段。
命名配置保存时同步 `latest`；加载命名配置时也更新 `latest`，供下次启动恢复。
配置缺失或格式错误会报告失败，加载失败不会创建同名文件；启动时损坏的 `latest`
不会被自动保存覆盖。写入采用 UTF-8 和临时文件替换。

每 100 个客户端 tick 检查状态变更并自动保存，覆盖按键切换及游戏控制设置中的改键。
退出请求时立即保存并冻结当前参数，再关闭模块；GUI 的关闭回调不会覆盖退出前的启用状态。
重复退出回调不会重新采集参数；JVM 关闭钩子写入冻结快照，也可重试退出时失败的写入，避免在退出线程读取游戏对象。
非正常终止可能丢失最后一个自动保存间隔内的变更。

## ImGui ClickGUI

界面代码位于 `src/dev/myth/UI/`，绘制使用 imgui-java 的 GLFW/OpenGL3 后端。
默认按 **Right Shift** 打开；**Esc** 或打开键关闭，开关淡入淡出 150 ms。
需要 Minecraft 的 OpenGL 图形后端。

对照 `skid/openmio` 的 `PresetEnumSettingHelper`、`ArrayListPresetHelper2`、
`FontsSearchHelper4_2` 和 `nick/SettingValues` 实现分类面板、紫色透明主题、
尺寸、动画和控件。保留项目既有的 Combat、Misc、Render、Player、Client 五类。
分类标题前使用 Google Material Icons 的准星、拼图、人物、眼睛、齿轮图标；HUD 和音效分类也有对应图标。
图标与标题作为整体水平、垂直居中，随面板统一缩放和淡入淡出，已打包到 Mod，无需运行时联网；资源来源和 Apache 2.0 许可见 `META-INF/NOTICE-material-design-icons`。

- 左键拖动分类标题，右键折叠分类；滚轮移动面板。
- 左键切换模块，右键展开参数；`Ctrl + F` 搜索，Tab 补全，Enter 展开匹配项。
- 布尔参数左键切换、右键展开子项；数字左键拖动、右键输入；枚举左键循环、右键展开。
- 颜色右键展开 HSV、透明度、Copy/Paste、Rainbow 和 Sync；音效左键选择、右键试听。
- 音效选择页支持分类、拖动、折叠和滚动；拖入 OGG 会复制到 `config/myth/sounds/custom/`。
- Bind 行直接显示 `Toggle F8`、`Hold F8` 或 `ReverseHold F8`，捕获时显示对应模式和 `...`。
- 左键进入或取消捕获，捕获时按键盘键或其他鼠标键设置绑定，Esc/Delete 清除；未捕获时右键循环 Toggle/Hold/ReverseHold。
- 与 Mio 一样，UI 打开键固定为 Toggle；Fonts 和后续模块支持三种模式。
- UI 的 Position 支持 Custom、Left、Center、Right。默认 Custom 保留拖动布局；选择预设会覆盖自定义位置，
  将五个分类按固定顺序放到左侧、水平居中或右侧，保留 5 的分类间距，并随窗口尺寸、GuiScale 和 FrameWidth 调整。
  拖动分类或滚动面板会切回 Custom，选项和位置随配置保存。
- UI 的 Blur 开关使用横向、纵向两遍高斯模糊，右键展开可设置 Strength（1～20，默认 6）与 FPS（1～240，默认 60）。
  FPS 只限制模糊背景刷新，控件和文字仍每帧绘制；改变强度、开关、窗口尺寸会立即刷新。
  沿用 Mio 的局部范围：分类面板、音效分类和提示框后面的背景，HUD 编辑器只模糊其分类面板。
  面板折叠、移动和缩放时范围同步变化。全屏紫色 Tint 和 Gradient 铺底已移除。
  Strength 与 FPS 同时作用于 HUD 的模糊背景：缓存纹理只有一份，但 HUD 有独立开关，
  `HudElement.wantsBlur()` 为真的组件会让模糊在普通游戏画面里也准备，不受本开关影响。
  开启模糊时一帧分两遍提交：先画不要模糊的 HUD，再取样后台缓冲，最后画模糊组件、HUD 编辑器
  面板和 ClickGUI，所以压在它们下面的 HUD 会被一起模糊而不是被盖住。
  两遍之间 `ImGui.isMouseClicked` 只在第一遍成立，点击沿由 `Myth.UI_MANAGER.isMouseClicked(button)` 锁存提供。

Client 中的 **UI**、**Fonts**、**HUD** 行是界面自身的设置，未向 `ModelManager` 注册功能模块。
字体库 `UI/font/FontHelper.java`、`AdvanceGlyph.java` 来自 Mio，保留 AWT 字形生成、
字符宽度、格式颜色、半尺寸绘制与阴影规则，纹理提交适配 ImGui。
Fonts 默认开启，使用 Verdana 17 并开启 AntiAlias；关闭时使用 Minecraft bitmap，
支持 Font、Style、AntiAlias、Size、Shift、Shadow、Translate。
标题与选项按亮色字形本体的可见高度垂直居中，阴影不参与居中范围；Shift 可手动调整自定义字体的位置。
原版字形增加透明间隔，垂直坐标按实际屏幕像素对齐，保留居中计算产生的半个 GUI 单位，
避免文字向上偏移及悬停动画采样到相邻字符而闪出白线。
Line 使用 Mio 的四条平面填充边框；右键展开 Elements → Line → Color 可独立设置颜色、透明度、Rainbow 和 Sync，
默认关闭 Sync，可直接自定义；开启 Sync 跟随 UI 主题。分类、枚举展开、音效和 HUD 编辑器面板共用该边框颜色，随配置保存。
外侧边框先绘制与面板相同的 BgColor，模糊也覆盖边框；Line 与 Color 设置相同 RGBA 时使用相同的颜色叠加方式。
悬停音效仅在进入控件时播放，点击每次按下触发一次。
Verdana 由系统字体提供；Mio 本身未附带 TTF。16 个 OGG 音效直接来自 openmio 资源。

布局、面板叠放顺序、展开状态、UI 选项、字体和绑定写入既有配置的 `ui` 字段；
模块 `settings` 和绑定模式也随配置保存。聊天命令和 GUI 使用同一份状态。
新增参数可通过 `Model.addSetting(...)` 添加 `UI.setting.Setting`，
不需要修改 GUI。界面资源没有语言 JSON。

## HUD 框架与组件

右键展开 **Client → HUD**，左键点击 **HudEditor** 进入编辑器，Esc 返回原界面。
对照 openmio 的 `HUDSearchHelper4`、`ModuleListMode`、`SearchHelper4_10` 和
`ZoomHelper_2`，使用现有 ImGui 后端实现 HUD 标题面板、白色中心参考线和安全边距编辑。
面板初始位置为 `(85, 10)`，左键拖动、右键折叠、滚轮移动；从屏幕边缘拖动可调整 SafeX/SafeY。
复用 ClickGUI 的字体、主题、缩放、MSAA 与 150 ms 开关动画。

`Myth.HUD_MANAGER` 管理六种锚点和组件顺序；`UI/hud/HudElement` 为后续组件的抽象基类，
提供尺寸、启停、参数与归一化位置。框架包含组件拖动、中心吸附、边角锚定、方向键微调、
I 互换左右分组和 Delete/Backspace 禁用逻辑。状态写入配置的 `hud` 字段，旧配置仍可加载。

HUD 编辑器现在列出 **ModuleList**（Mio 的 ArrayList），默认关闭、锚定右上角。
左键开关，右键展开 OnlyBound 和 Sort（Length / Alphabet）；仅显示启用且允许显示的功能模块，
Notifications、UI、Fonts、HUD 和 ModuleList 自身不入榜。OnlyBound 仅保留已绑定键鼠的模块。
支持 Mio 的四次方滑入/滑出、渐隐、动态行高，以及左右对齐、上下排列、中心对齐和自由拖动。
`Model.getInfo()` 可提供灰色括号内的状态后缀；原版和自定义字体均支持后缀格式与宽度计算。
正常 HUD 使用纯文字，不增加列表背景或侧边线；编辑器的选中框、拖动、吸附和配置沿用 HUD 框架。

Client → HUD 中增加 SmoothWidth 和 Colors，采用 Mio 的默认颜色 `#BD99FF`、
None / Pulse / Raw、FadeOffset、SecondColor、ColorAmount（2～9）、FadeSpeed、MixColors 与彩色序列。
颜色支持 Rainbow / Sync；模块列表参数、HUD 配色及锚点和位置均写入现有配置。
移植参考 `module/ModuleList`、`ModuleListCryptoHelper`、`CryptoHelper`、`ModuleListSearchHelper4_2`、
`HUDHelper_2`、`module/client/HUD`、`MixinTitleScreenMode`，渲染提交适配现有 ImGui 后端。

HUD 编辑器同时提供 Mio 的 **Watermark** 与 **Welcomer**：Watermark 默认开启、锚定左上角，
Text 默认 `Myth`，显示本项目实际版本；VersionColor 默认开启，让版本部分显示白色，关闭后跟随 HUD 配色。
Welcomer 默认关闭、居于屏幕中心的自由位置；Mode 支持 Name / UID / Custom，Name 显示
`Hello 玩家名 :')`，Custom 的 Text 默认 `Welcome to Myth.`。本项目没有 Mio 账号系统，UID 保留
openmio `WelcomerHelper` 未初始化时的 `-1`，不生成账号或伪造 UID。
两者复用 Mio 文字滑入、SmoothWidth、HUD 配色、原版/自定义字体、拖动锚点和现有配置保存。

**Map** 只有一块模糊背景和中心的玩家圆点，不采集世界数据、不画区块或 ESP。
Width（50～500）、Height（50～250）与 Size 决定面板大小，Background 为叠在模糊上的面板底色，
Outline 是面板外侧的一像素边框。Blur 默认开启，复用 ClickGUI 的 `GaussianBlur` 缓存和
Client → UI → Blur 的 Strength / FPS，但有独立开关：它为真时模糊在普通游戏画面里也会准备，
不要求界面打开或 UI 的 Blur 开启。Dot 默认红色 `#FFFF0000`，支持 Rainbow / Sync，
Radius 为 0.5～8 并随 Size 缩放，超出面板的部分按面板裁剪。
HUD 允许自由叠加：Map 压在别的 HUD 上时会把它们模糊进自己的背景，而不是盖掉。
