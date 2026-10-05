# Leader Prediction → Nova AntiKB

参考：`skid/Leader Lite-B0.4.jar`，SHA-256
`47194DCCF7950451F23B8542E3AE8356F32A1A51FE37140DB064EFB243BBBBC8`。
在 Recaf 现有工作区核对了 `Velocity`、`KillAura`、`BlinkManager`、`DelayManager`、
`MixinEntityPlayer`、`MixinEntityPlayerSP` 及相关工具方法。没有切换参考 JAR，也没有新增 Java 源码。

## 入口

AntiKB → Mode → **Prediction**，通过 Mode 旁的齿轮展开参数。
原来的 Delay/Reduce 仍为索引 0/1，Prediction 追加为 2，不改变旧配置含义。
此移植的 JNI 和字节码适配目标是 **Lunar 1.8.9**；其他版本显示相同设置，但不会启动该模式的包队列。

ReduceMode 四个选项：

| 模式 | 参考行为 |
| --- | --- |
| Attack | S12 后按计数窗口尝试 3 格玩家射线；发送 AttackEvent、C0A、C02，再将自身 X/Z 乘 0.6；可保留疾跑 |
| ReleaseWhenCanAttack | 在 Hypixel 对应可攻击阶段释放入站队列 |
| ReleaseBeforeCanAttack | 在 Hypixel 对应攻击前阶段释放入站队列 |
| Blink | 按 hurtTime 启动出站缓冲；可要求 KillAura 正在格挡；按收到击退后的 tick 释放 |

保留 Reduce、ExtraAttack、Only Sprinting、SmartTimes、Attack Times、KeepSprint、
TestMode、Jump、Delay、Delay Ticks、Force Delay Rising To Falling、Delay Till On Ground、
Ground Delay、Rotate、Rotate Ticks、Auto Move、Fake Check、Debug 及各分支条件显示。
默认值与参考一致；数值使用整数格式。

## 包与 AutoBlock 联动

- 入站缓冲持有触发它的 S12 和后续有序数据；S00 keepalive 放行；S19 缓冲本人的 hurt opcode 2 以及实体尚未解析的状态包。
  Join/Respawn 先释放，Disconnect 丢弃旧队列。已有 Backtrack 在接收自身击退前释放，并避让 AntiKB 的入站所有权。
- S19 hurt 解锁 Fake Check；实际应用 `setVelocity` 后计算 SmartTimes、旋转和跳跃状态。
- Jump 使用与参考相同位置的 `onLivingUpdate` 父类调用前钩子；Auto Move 使用更新移动输入后的钩子。
- Hypixel 共享状态机提供 `velocityCanReduce` 阶段判定、`stoppedBlock`、取消 Aura 攻击及 ExtraAttack 的阶段重置。
  无 Hypixel 状态机时，采用参考非 Hypixel 路径的可攻击判定及独立格挡范围目标扫描。
- Velocity Blink 可以接管 AutoBlock 已有出站队列；AutoBlock 的结束请求不能释放 Velocity 拥有的队列。
  接管不提前发出旧包。Keepalive/聊天放行，空队列的 C0F 放行。
- Blink 期间原版攻击方法的 X/Z 减速写入和取消疾跑被抑制；重放每个 C02 时才乘 0.6。
  参考判断的是整个 C02 类，**INTERACT 和 INTERACT_AT 也会计入**，这里保留该行为。

## 特殊行为和集成边界

以下不是根据模式名称重新设计的行为，而是保留参考代码的实际控制流：

1. `Delay Till On Ground` 默认为 true，其入站开启条件不受外层 `Delay` 开关限制。
   因此默认关闭 Delay 仍可能在空中进入缓冲，落地释放。
2. SmartTimes 将实际 double 速度先转成 int，再进入公式；常见速度小于 1 时结果通常为 3。
   没有擅自换成 S12 的原始整数单位。
3. Attack 的 reduceTick 初始为 -1，达到次数后清标记也不会立即退出本次尝试；射线落空仍消耗次数。
4. ReleaseWhen/Before 的阶段释放不要求 delayFlag 为 true。
   ExtraAttack 的 `velocityAttacked` 只在 Attack 分支消费；参考在另外两个分支会留下标记，不能将它描述成必定即时补打一刀。
5. `StartReleaseTicks` 从收到击退开始计数，不是从 Blink 开始计数。
6. 原 JAR 的 `Velocity.onTick` 参数是 Forge `TickEvent`，Leader 的事件总线按具体类匹配，实际发出的是
   `leader.events.TickEvent`。因此其 TestMode 计时体没有对应的正常客户端事件入口。
   本移植保留方法逻辑和参数，但**不擅自把这个原本不执行的计时体接到另一事件**；TestMode 不应作为正常减速触发方式。

Nova 的必要集成差异：

- 修复参考释放 Blink 后仍可能残留 VELOCITY owner 的问题；关闭、子模式切换和世界变化均清理所有权及 JNI 引用。
- 关闭/切换时释放入站队列，丢弃属于 Prediction 的出站队列；后者与参考 disable 一致。
  世界变化/断线/死亡丢弃旧世界入站包，不向新世界重放。位置纠正会结束当前 Blink。
- 入站队列有 8192 包上限，JNI 分配失败时释放已有队列并放行；正常顺序下不会改写包内容或伪造确认。
- Prediction 的旋转只持有当前 tick；结束后归还 Nova RotationManager，避免锁死视角。
- Nova 现有 Stuck 开启时不启动新的入站延迟。未额外移植 Leader 的 LongJump、KeepSprint 独立模块或 BlinkSettings 慢放模块；
  它们采用参考关闭时的路径。Prediction 内部的 KeepSprint 选项已移植。
- Debug 输出到 Nova 日志，不向服务器发送聊天消息。
- 为 DelayManager 的计数增加了 runTick POST；同步修正 KillAura 不能在 POST 清空已选目标的问题。

核心状态机在 `src/minecraft/modules/combat/antikb/prediction/PredictionVelocity.h`，JNI/事件适配在 `PredictionRuntime.cpp`，
所有 Lunar 映射和新增钩子仍集中在 `src/minecraft/compat/mappings/Lunar189.inc`。

近期适配修正及测试详情见 [对齐复查](leader-prediction-audit.md)。

## 验证

`test/combat/antikb/run_prediction.ps1`：四个 Reduce 分支、Fake Check、计数边界、延迟条件优先级、Blink、跳跃和旋转；
提取生产出站队列代码验证接管、所有者隔离、FIFO、每个 C02 的减速补偿、字段写入钩子及 JNI 引用清理；
提取生产入站代码验证包例外、重放、分配失败、Stuck 和世界/死亡/断线清理；验证真实设置定义、显隐和配置序列化。

`run_hypixel.ps1`：真实 Lunar 缓存的相关成员绑定及 LWJGL 输入绑定、Minecraft/EntityPlayerSP/EntityPlayer 的生产字节码编辑；
校验跳转、异常表、栈映射和双槽 PUTFIELD 描述符检查；Hypixel 包序列及 480 组持续状态机回归。

`run_antikb_delay.ps1`、`run_autoblock.ps1`、`run_killaura_cps.ps1`：旧模式回归。
Release 构建检查没有反射加载器不支持的 PE 静态 TLS。

这些是离线验证，没有代替实际注入和服务器测试；由用户注入 DLL 测试。
# 入站队列超时修复

Prediction 的入站延迟现在受 `Delay` 总开关控制，隐藏的 `Delay Till On Ground` 不再独立启动扣包。持续等待落地/下降最多允许 500 毫秒或 10 个游戏 tick，由入站回调与 GameTick PRE 双入口检查，不依赖 PlayerTick POST。位置纠正和事务确认包到达时，先按 FIFO 回放已有队列，再放行当前包；队列满或引用分配失败时同样释放并放行。关闭 Delay 会立即释放已有队列。这些连接存活保护优先于参考客户端的无限等待行为。
