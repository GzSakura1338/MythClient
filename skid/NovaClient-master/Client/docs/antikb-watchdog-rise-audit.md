# Watchdog Reduce 与 Rise 对照

更新：2026-09-20。参考：
`skid/Rise-6.9.5-main/src/main/java/com/alan/clients/module/impl/combat/velocity/WatchdogReduceVelocity.java`。
按用户最终要求，对齐正常收包和同步释放逻辑，保留入世界恢复、S08 特殊处理；
Slinky/Hypixel 的互斥由 AutoBlock 负责，AntiKB 仅提供只读状态。

| Rise 行号 | 行为 | Nova 实现 |
| --- | --- | --- |
| 21–26 | 三个选项默认 true，FIFO 与 flushing | 一致；Cancel Explosions 同样未被读取 |
| 28–37 | 非 flushing 时保存所有自身 S12 | 恢复保护通过后相同；没有速度方向过滤 |
| 38–51 | 非空队列捕获 S32 和 z | 仅捕获 S32；取消移植版的 S18 延迟，避免与立即通过的 S14 相对移动乱序 |
| 28–54 | 忽略原入站事件 cancelled 状态 | 相同，重新回放时发布新事件 |
| 56–63 | ATTACK 回调内同步释放，忽略取消状态 | 入站回放相同；原生出站适配器按物理阶段安排确认包 |
| 65–71 | 真实 onGround 的 PRE Motion 同步释放 | 相同 |
| 77–86 | 触发时取走整个队列 | 相同；之后的新 S12 留在新批次 |
| 88–96 | flushing 包围每包 receive | 新入站事件、原处理器和整批重入保护 |
| 100–102 | 关闭时同步回放 | 相同；退出该模式也按关闭处理 |

S08 前回放旧输入、出生/纠正恢复、世界身份和 JNI 生命周期保护是保留的差异。
远端 S18 绝对位置与 S14 相对移动现在都立即通过；这是修复其他玩家遁地/瞬移的主动差异。
AntiKB 没有普通超时、按队列容量释放或下一 tick 入站释放。
AutoBlockPacketSync 与原生适配器读取 AntiKB 状态，结束或拒绝 Slinky/Hypixel 的出站延迟。
其他模式已有的队列不会被该互斥策略主动释放。

本次修复保留同步入站回放，但不声称出站实现与 Rise 完全一致：原生层暂存动作组，
避免 C0F 插入 C0A/C02 之间。攻击回放发生在物理更新前时，确认先于动作组和本次移动；
落地 PRE Motion 在物理更新后时，确认跟在当前 C03 后面。没有 C03 时，下一 tick HEAD
先发送确认再进入物理更新，动作仍等待真实移动边界。未完成动作组也会短暂阻止模式切换
接管出站队列；完成后恢复正常。这些是为 Nova 的回调阶段添加的适配差异。
离线用例覆盖事务与移动窗口，并不代表已经证明服务器上不会回弹。

验证与最终实现说明见 [Watchdog Reduce](antikb-watchdog-reduce.md)。
旧 `build-vs2026/test-watchdog-release/audit.cpp` 是旧源码复制夹具，不能代表当前版本。
