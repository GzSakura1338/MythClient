# AutoCrystal（Mio 核心逻辑适配）

## 独立模块

Combat 下的 `AutoCrystal` 和 `LegitCrystalAura` 是两个独立模块，各自保存参数、热键、运行状态、选点和确认记录。新实现位于 `src/minecraft/modules/combat/autocrystal`，原模块的选点/攻击算法不替换。两者同时开启时 AutoCrystal 获得优先权，LegitCrystalAura 暂停交互；关闭 AutoCrystal 后旧模块继续运行，不改变其启用参数。

沿用 C++/JNI，不新增 Java 类。支持现有 Crystal JNI 的 Lunar 1.21.11 / Minecraft 26.3；其他版本自动禁用并提示。

## 对照来源

参考 `skid/OpenMio-main/src/main/java/me/mioclient/module/combat/AutoCrystal.java` 及其辅助类。

| Mio 来源 | Nova AutoCrystal 行为 |
| --- | --- |
| `getAutoCrystalData1131` | 以眼睛位置构建范围网格，每个位置比较范围内所有合格玩家，采用完整扫描结果 |
| `AutoCrystalData.getAutoCrystalData11`、`AutoCrystalDataMode` | 直接放置优先于遮挡位置，遮挡位置优先于补黑曜石；相同阶段选最高目标伤害 |
| `getAutoCrystalData1131` 的旧位置分支 | 与保留位置比较时使用 Java round 对应阈值，减少位置抖动 |
| `getAutoCrystalData_41147` | 排除自己、队友、机器人、死亡、旁观者和创造玩家；新底座不得超过目标脚部高度加 1.5 |
| `AutoCrystalMode_3.get847` | 最低伤害不超过目标剩余血量与吸收之和 |
| `ArmorSearchHelper4.get1900/get1902/get1903` | 强度 6 爆炸、包围盒曝光采样、难度、护甲韧性、抗性、爆炸保护；规划底座作为虚拟实心方块参与曝光 |
| `HoleSnapHelper.holeSnapHelper2` | 伤害射线忽略抗爆性低于 600 的方块；不修改世界方块，仅跳过被忽略的体素 |
| `AutoCrystalSearchHelper4.is2070` | 水晶占位与预测水平占位检查；底座同时检查当前/预测目标包围盒的并集 |
| `do1132` | 提交前复查预测伤害；预测伤害大于 2 但低于最低值时尝试当前伤害回退 |
| `do1133` | 12 个内部采样点选择最近可交互表面，已合法的当前射线可以保持 |
| `do1134/is1151` | 自动切黑曜石、等待底座成为真实方块后接水晶，优先复用仍可造成有效伤害的已建底座 |
| `do1135/do1136` | 按预测伤害选择可引爆的真实水晶，单独控制攻击间隔；其他水晶存在不会全局禁止工作 |
| 主手/副手与 autoSwap | 主手热栏自动切换、持续持物、Silent、本地动画开关、副手水晶；停止工作恢复槽位且尊重手动滚轮 |

## 简化参数

- Target：Range（默认 4.5）、FOV（360）、Through Walls。
- Place：Delay、Auto Switch、Auto Base（含 Place Delay）、Silent。
- Break：独立 Delay；Swing 控制本地挥手动画。
- Rotation：Speed。使用 Nova RotationManager 的静默独占转头，自动启用已有移动修复。
- Safety：Only Above Crystal（默认关）、Ignore Self Damage、Min Target Damage、Max Self Damage、Min Remaining Health。
- Place ESP：使用项目现有 ImGui 放置提示。

预测固定为 2 tick、目标扫描上限 12 格，保留实际交互距离检查。没有引入 Mio 多组交互模式、猜测实体 ID 的 Factor 或多种库存换物参数。

## Nova 调度与对齐边界

这是按 Mio 核心逻辑实现的独立本地适配，**不是全部行为 100% 等价的声明**：

1. Mio 的设置初始化在 native loader 中，当前参考 Java 不包含完整可核验的默认值；`AutoCrystalMode_3/7` 等反编译方法有错误标记，不能把这些文本视为完整原始源码。
2. Mio 在渲染线程/执行器计算计划、可在收包回调即时攻击并顺序补放。Nova 在游戏线程分批计算，每 tick 最多扫描 128 格、4 个详细候选，2 ms 软预算；提交前重新验证。JNI 世界查询不搬到异步线程。软预算不能中断正在进行的一次 JNI 查询。
3. 预测复用 Nova 原生的重力/碰撞外推，未照抄 Mio 反编译器产出的异常运动表达式；不等同于其完整运动预测器。
4. 保留 Nova 自伤/剩余血量安全阈值，没有复制 Mio 的图腾强杀自伤豁免、强制自杀、装备耐久触发 FacePlace、SpeedMine 联动、入服无敌推断、抗虚弱切武器及挖火流程。
5. 只攻击世界中真实出现的水晶，不猜测未来实体 ID，不从网络线程直接发包。底座、水晶生成与移除均有有界确认/重试；没有实现 Mio 的所有 Instant/Sequential/Factor 组合。
6. Through Walls 只放宽本地射线遮挡，不代表服务器接受穿墙交互。没有模拟多种服务器的所有检查，也没有承诺 Grim 不产生 VL。

## 转头、模块协调与协议检查

通过 `SetExclusiveRotations` 请求转头，保留灵敏度量化、连续 yaw 和现有移动修复。放置使用 `SubmitItemUse(owner)`，攻击使用 `AcquireAttack/BeginAttackSubmission`，同 tick 不交叉提交不同拥有者的攻击/物品交互。角度由统一事务冻结，避免 use-item 与 motion 使用不同角度。Motion 后不补发交互。

已阅读本地 Grim `BadPacketsJ` 和 `DuplicateRotPlace`，沿用现有精确角度事务及放置转头历史门控。未修改 RotationManager 的既有 AimModulo360/手部插值修复。

2026-09-30 修正 DuplicateRotPlace：AutoCrystal 不再从 MotionEvent 推断上次放置角度。新增 `PlacementWire.h`，在现代出站钩子完成取消判定后记录实际包顺序，包含普通、重放及 no-event 包。收到移动包时先消费排队放置，再记录本包的 look delta，与 Grim `handleFlying` 的顺序一致；没有 look 的位置/状态包不清除旧转角，显式相同角度的 look 包则记录零转角。模块开关不重置连接历史，初始未知历史先等待小幅转角；无法确定是否产生放置的 USE_ITEM（水桶/烟花等）保守失效历史。`nova-base.log` 增加 `wire[...]` 字段便于复核。

AutoMLG、AutoMace、AutoCharge、Scaffold、库存救援、AutoBlock 占用和延迟发包会使本模块让出。与 Anchor 共用伤害仲裁；执行动作时让 KillAura 暂停，避免另一个攻击改变本次旋转。FeetPlace 能读取本模块的确认状态。

## 验证

`Client/test/combat/autocrystal/run.ps1` 编译真实新模块并验证：多玩家按伤害选择、完整扫描取优、阶段/旧位置阈值、低血量阈值、抗爆射线、底座衔接、放置/攻击确认去重、主副手、Silent、创造/队友/自伤过滤、穿墙开关、手动槽位、关闭/救援/换世界清理。

`Client/test/combat/legitcrystalaura/run.ps1` 验证原模块及 AutoCrystal 启用时暂停、关闭后恢复。`Client/test/utils/rotation/run.ps1` 验证转头连续性、移动策略、独占攻击和物品事务。`mapping.ps1` 针对两版真实 JAR 检查 Crystal 层绑定；Lunar 抗爆性方法另经 Recaf 读取字节码。

构建目标：`NovaClientBase`，输出 `Client/build-vs2026/Release/NovaClientBase.dll`。本次未自动注入，尚未做服务器实战验收。
