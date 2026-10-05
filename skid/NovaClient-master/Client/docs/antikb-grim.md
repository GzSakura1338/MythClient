# AntiKB — Epsilon Delay / Reduce

AntiKB 提供 Epsilon Velocity 的 Delay / Reduce 两个模式，通过现有游戏适配层识别和回放数据包。独立的 Velocity 模块不变。默认继续使用 Delay，原配置无需添加 Mode 才能工作。

参考：`skid/Epsilon-Private-26.2/common/src/main/java/com/github/epsilon/modules/impl/movement/Velocity.java`，`delayPacket`、`onDelayTick` 和 `onKeyboardInput`。

## 设置

- Mode：Delay / Reduce，各模式仅显示自己的参数。
- Delay / Delay Ticks：1–5，默认 3。
- Delay / Jump Reset：默认关闭；开启后在地面且 `hurtTime == 9` 时设置当次跳跃输入。
- Reduce / Attack Counts：1–5，默认 1。
- Reduce / Sprint Ticks：1–5，默认 3。
- Reduce / Max Delay：0–1000 ms，步长 50，默认 1000。
- Reduce / Swing Hand：默认关闭；关闭时发送挥手包，开启时执行本地挥手。
- ArrayList 后缀：Delay 显示剩余玩家 tick；Reduce 缓存时显示经过的毫秒数。

移除 AntiKB 原有的 Delay 开关、毫秒延迟、Attack reduce、Release autoblock、Only moving 和 Cooldown ticks。旧配置中的无效设置不再使用；已有的 Delay Ticks 和 Jump Reset 值仍按原名加载，所以旧配置可能保持 Jump Reset 开启。

## Delay 时序

1. 收到自己的速度包，且 Y > 0、X/Z 至少一个非零，并距离最近位置纠正或爆炸至少 100 ms 时，启动倒计时。首次触发的速度包正常处理，不取消、不缓存。
2. 倒计时期间，缓存后续所有入站包，保持到达顺序，包括后续速度、位置纠正、爆炸和重生包。位置纠正与爆炸更新 100 ms 保护时间，不提前释放队列。
3. 在低优先级 PlayerTickEvent（玩家更新前）递减计数，归零后按 FIFO 回放。GameTickEvent 不递减计数，后续速度包不重启窗口。
4. 断线包直接通过，待处理队列丢弃。关闭模块时回放；退出本地世界没有收到断线包时，额外丢弃失效队列。

## Reduce 时序

1. 收到符合正 Y、非零水平分量和 100 ms 保护期条件的自身速度包时，连同首次速度包一起缓存。延迟期间仅缓存速度、实体相对移动、实体传送、Ping、LookAt、玩家位置纠正这六类包，后续包不重启窗口。
2. 爆炸包直接通过；位置纠正包在窗口内缓存。两者都更新时间保护值。每个自身速度包都会更新击退 yaw，包括保护期内或不符合击退启动条件的自身速度包。
3. ClientTick.Pre 时，若落地、距位置纠正/爆炸不足 100 ms，或达到 Max Delay，则先回放队列再设置跳跃请求。仅在保护期之外创建攻击和前进计数。Max Delay=0 仍是在下一次 ClientTick.Pre 释放。
4. 有前进计数、KillAura 没有目标且 Scaffold 未开启时，以速度 180 和最高转向优先级请求击退 yaw，保持当前 pitch。KillAura 缓存期间按 `abs(velocityYaw - targetYaw)` 排目标，忠实保留参考源码未 WrapDegrees 的排序。
5. 每个低优先级 PlayerTick.Pre 最多执行一次攻击尝试：射线命中存活玩家时按协议排列攻击和挥手，Lunar 1.8.9 先挥手再调用原版 attack，1.20.1 / 26.3 先 attack 再挥手；本地挥手和仅发送挥手包均遵守该顺序，避免 1.8 的 PacketOrderB pre-attack。无目标、非玩家、已死亡都会消耗一次计数，不把攻击转交 AutoBlock 改期。KillAura 在计数未耗尽时暂停自己的攻击和追加攻击计数；启用 AntiKB 时，Hit Select 不使用自身 hurtTime>=6 的提前攻击分支。
6. 延迟期间，fallDistance=0 且射线命中玩家时强制前进/清除横移；释放后在首次落地输入中消费一次跳跃请求，保护期内只消费、不跳。Sprint Ticks 每次输入都会消耗，只有无 Aura 目标、Scaffold 关闭且保护期外才强制前进；它不直接调用 setSprinting。

## 生命周期和适配边界

计数与 JNI 全局引用队列共用互斥锁，游戏回放在锁外执行。模式切换按 Epsilon 的回调语义丢弃旧模式的包和计数；关闭模块回放，断线/退出世界丢弃并释放 JNI 引用。不会接管 AutoBlock 的出站队列，也不会主动解除格挡。

Nova 的旋转管理器通过 PickCursor 更新原版逻辑射线结果，Reduce 使用该结果，保留游戏自身的距离和遮挡判定。1.8.9 没有 LookAt 包，因此白名单只包含该版本实际存在的包；26.3 的 Reduce 使用独立白名单，不更改旧 Velocity 的白名单。

Backtrack 已重做为仅含 Delay / ESP 的独立模块，只跟踪实际攻击的玩家，通过 BlocksBacktrack 避让 AntiKB 的 Delay / Reduce 队列和攻击计数。Backtrack 收包优先级高于 AntiKB，自己的击退包到达时先释放旧队列，再由 AntiKB 接管；不改变 AntiKB 的目标策略。详见 backtrack.md。Nova 仍没有独立 Criticals 或全局 AntiBot 模块；AntiKB 的目标判断对应 Epsilon AntiBot 关闭时的行为。跨版本游戏物理、攻击和格挡结果不能宣称完全相同，实际服务端效果由用户测试。

## 验证

`Client/test/combat/antikb/run_antikb_delay.ps1` 使用实际 AntiKB 源码、实际事件总线及模拟游戏/JNI 接口，覆盖 Delay 的 1–5 tick、Reduce 白名单/FIFO、保护期、计数、攻击/挥手顺序、移动输入、模式切换与生命周期。AutoBlock 原有模拟测试另外执行；26.3 映射通过 audit_mapping.py 对实际 JAR 校验。

产物：`Client/build-vs2026/Release/NovaClientBase.dll`。用户自行卸载旧 DLL、重新注入并实测。
