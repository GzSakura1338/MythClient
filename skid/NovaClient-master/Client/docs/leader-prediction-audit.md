# Leader Prediction / Hypixel 对齐复查（2026-09-19）

参考 Recaf 当前工作区的 `skid/Leader Lite-B0.4.jar`，SHA-256 为
`47194DCCF7950451F23B8542E3AE8356F32A1A51FE37140DB064EFB243BBBBC8`。
核对 Velocity、KillAura、DelayManager、BlinkManager、UpdateEvent、RotationUtil、RayCastUtil、
PlayerUtil、KeyBindUtil、Disabler 以及相关 Mixin。以下是审查后已经实施的修正；实现和测试没有新增 Java 源码。

## Prediction 已修正

| 差异 | 修正后的行为 |
|---|---|
| Rotate 后用新角度作攻击射线 | 每个 Player PRE 读取真实 lastReportedYaw/Pitch；旋转计算、攻击射线共用固定快照 |
| 射线使用 FrameTime | LeaderRayCast 固定 EyePosition(1)，只按 canBeCollidedWith 筛选，不排除同载具；其他模块的默认射线语义不变 |
| 移动修正先改变横移输入 | Prediction 持有旋转时关闭保方向修正，移动/跳跃使用该旋转；释放后恢复 RotationManager 默认设置 |
| 潜行 Auto Move 变成 0.3 | 写入最终 forward=1，保留原横移和潜行标志；只在原版规定的 rotateCounter 窗口生效 |
| 非 Hypixel 根据已选攻击目标决定放包 | 独立扫描 loadedEntityList，按目标过滤和 AutoBlock Range 判断，不受选中目标、FOV 和 KillAura 启用状态限制 |
| 未解析实体的 S19 直接放行 | 未解析 S19 作为普通延迟包，本人 hurt=2 继续延迟，其余已解析 S19 放行 |

核心四个 Reduce 分支、SmartTimes 截断、终止标记清除后继续当次攻击、Delay Till On Ground 优先级、
每个 C02（含交互）的回放减速保持参考逻辑。TestMode 的 Forge TickEvent/Leader TickEvent 类型不匹配是原版行为，
没有将该原本不执行的计时方法接入普通 tick。

## Hypixel AutoBlock 已检查和修正

- 核对 OldHypixel、Without NoSlow、Custom 以及五种 Lag 时序：放包、切槽、C07、攻击、交互格挡、POST 格挡及 APS 累积。
- 修正独立 AutoBlock 的点击捕获范围：与更新逻辑共用 Hypixel Range，不再提前使用普通模式的 Range。
- KillAura 在 GameTick PRE 保存目标膨胀包围盒和位置。挥手距离、攻击射线与 INTERACT_AT 都读取这份快照，避免在一个周期中使用不同目标位置。
- 射线从盒内出发时使用 calculateIntercept 返回的出射面；不再伪造眼睛位置作为交互命中点。整条线段留在盒内时不会凭空算命中。
- Require Right 跟随实际“使用物品”绑定读取物理输入，包括改绑键盘或其他鼠标键；打开界面时为 false。
- Start/Stop/Interact 顺序已核对：sync → C08 → 本地使用；C07 → 停止本地使用；sync → INTERACT_AT → INTERACT → C08 → 本地使用。
  Attack 复用 1.8 控制器的同步槽位、C02 和本地攻击路径，保留现有 AttackEntityEvent 钩子及 Prediction Blink 的减速写入保护。

## 保留的集成边界

移植对象是 Prediction 和 Hypixel AutoBlock，不是整个 Leader 客户端：

- 保留用户要求的统一 AntiBot/Teams 和全部模式的 Force block animation；视觉姿态不改变实际格挡统计。
- KillAura 仍使用 Nova 的目标类型、选敌/旋转设置和非格挡 CPS；独立 AutoBlock 仅根据手动攻击输入出手。
- 未移植 Leader 的 NoSlow、LongJump、独立 KeepSprint、SmartAttack、Disabler 第二把剑选择和 BlinkSettings 慢放；按它们关闭时的路径集成。
- 保留 Nova 的死亡/换世界/菜单清理、队列上限、JNI 引用和分配失败保护、旋转所有权释放。

因此可以确认上述遗漏已经修正，不能把整个客户端或服务器表现宣称为逐行为 100% 相同。

## 验证

`test/combat/antikb/run_prediction.ps1` 除核心和队列测试，还提取真实生产定义验证：

- lastReported 字段读取、S19 解析成功/失败分类、物理鼠标/键盘使用绑定、潜行最终输入。
- 通用射线/Leader 射线的插值、同载具、旁观者、碰撞资格、方块遮挡差异。
- Rotate 后射线继续使用原快照，移动修正所有权，AntiBot 过滤。
- 格挡目标扫描独立于攻击方向，目标快照、盒内出射面与 INTERACT_AT 位置。

`test/combat/autoblock/run_hypixel.ps1` 验证新增 Lunar 字段和 LWJGL 绑定、真实类字节码编辑、
包序列、480 组持续状态机用例和生产属性。
`test/run-offline.ps1` 包含 14 组离线回归；Release 构建附带反射加载器静态 TLS 检查。
以上不替代注入和服务器实测，游戏测试由用户进行。
