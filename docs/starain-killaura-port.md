# Starain KillAura 移植

参考 `skid/Starain-1.20.1-main/src/main/java/today/starain/module/impl/combat/KillAura.java`，
实现位于 `src/dev/myth/model/combat/KillAura.java`，默认关闭、未绑定，列在 Combat。

| 参数 | 默认 | 范围或选项 |
| --- | --- | --- |
| Hit select | true | 开关 |
| Attack Invisible | true | 开关 |
| CPS Mode | Default | Default、1.9+ |
| CPS | 12 | 1～20 |
| Wait Critical | false | 仅 1.9+ 可见，右键展开子项 |
| Interrupt Sprint | false | Wait Critical 子项 |
| Critical Sound | false | Wait Critical 子项 |
| Aim range | 4.00 | 3.00～6.00，步进 0.01 |
| FOV | 180 | 0～360 |
| Rotation speed | 180 | 0～180 |
| Mode | Single | Single、Switch |
| Switch delay | 100 ms | 0～500 ms，仅 Switch 可见 |
| Priority | Range | Health、FoV、Range、None |

Default 模式保留 GameTick PRE 选目标、排序、切换及转头，RenderSkiaEvent 按高斯 CPS 延迟排队，
PlayerTick 执行攻击与主手挥动，使用物品时不攻击。Hit select 的 hurtTime 判定保持源代码。
调用现有 `RotationUtils.calculate(..., true, range)`、`RayCastUtil.rayCast(...)` 和
`RotationManager.setRotations(..., rotation -> raycast target)`；MoveInput、Strafe、Jump、
Motion 与动画继续使用已有 RotationManager/MoveUtils 事件链，无第二套转头或移动修复。

Starain 的 Target 筛选作为 KillAura 的 Targets 参数组：Player 默认开启，Invisibles、Animals、
Mobs、Villager 默认关闭。排除自己、死亡实体、旁观玩家，实体种类沿用源判断并适配 26.2 包名。
源代码先检查 LivingEntity，所以原有 EnderDragonPart 分支实际无法匹配，移植中省略。

已接入 Scaffold：开启时清理 KillAura 目标与攻击队列，暂停攻击和疾跑中断。
项目没有 Teams、AntiBot、Velocity、Criticals；这些联动按未启用状态处理，
不加入它们的反击退队列或 Criticals fallTicks。

保留源代码 FOV/FoV 的有符号 `player.getXRot() - targetYaw` 比较，未改成另一套视角筛选。
Switch 计时保留源 TimerUtils 的零初值和 System.currentTimeMillis；攻击延迟保留
`1000 / max(0.01, nextGaussian(cps, 5))`。

适配安全边界：缺失玩家/世界/游戏模式时清理目标和队列，Respawn 清理旧实体；getTarget 防止索引越界。
UI 参数使用既有 Setting、配置、绑定和通知机制。数字控件增加可选步进，以保留 Aim range 的百分之一精度；
其他参数沿用原有步进与显示精度。

## NovaClient 等待暴击

参考 `skid/NovaClient-master/Client/src/minecraft/modules/combat/killaura/KillAura.cpp`、
`CriticalSprint.h` 和 `Client/src/minecraft/java/game/GameCritical.cpp`。
在 Combat → KillAura 选择 CPS Mode → 1.9+，再开启 Wait Critical；
右键 Wait Critical 展开 Interrupt Sprint、Critical Sound。三个开关沿用 Nova 的默认关闭状态。

1.9+ 使用原版 `getAttackStrengthScale(0)`，冷却完全充满且数值有限才攻击，
每次 PlayerTick 最多攻击一次，并清除旧 CPS 排队。保留 Hit select 的受伤时间判断，
打开界面、使用物品或没有目标时暂停攻击。

Wait Critical 在地面、水中和蜘蛛网中允许普通攻击；空中必须下落距离大于零，
且不在攀爬、失明、乘坐或疾跑状态。蜘蛛网逐格检查玩家整个包围盒，包含头部。
这些状态由 `utils/player/CriticalAttackState.java` 读取，非法几何或下落距离阻止攻击。

Interrupt Sprint 按 Nova 的 HIGH PRE GameTick / MEDIUM MoveInput 优先级：
有目标且处于 `0 < fallDistance < 1` 的空中下落阶段，清除一次前进输入；
已有 MoveFix 先执行，侧向、跳跃和潜行输入保留。
疾跑恢复及网络包仍由原版 aiStep 管理。关闭设置/模块、切换模式、打开界面、
重生或目标丢失会清除待执行中断。

Critical Sound 仅在本地玩家真正进入原版 `crit(Entity)` 回调时播放经验球拾取音效，
音量 1、音高 0.9。必须同时开启 KillAura、1.9+、Wait Critical、Critical Sound；
不从攻击或挥手回调重复播放。现有项目无 KeepSprint 或反击退攻击队列，
因此不引入 Nova 对这些模块的联动实现。

验证：独立 26.2 单人世界完成 65 项运行检查，覆盖原 KillAura、冷却等待、
地面/水/头部蜘蛛网、失明/攀爬/乘坐/疾跑限制、共享 MoveFix 优先级、
一次性输入中断、开启 Sprint 时原版疾跑停止与恢复、实际 crit 音效和配置保存。
正式 `gradlew build` 通过；验证代码仅位于 `build/killaura-validation`。
