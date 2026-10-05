# KnockBackBalancer

实现位于 [KnockBackBalancer.cpp](../src/minecraft/modules/movement/knockbackbalancer/KnockBackBalancer.cpp)，在具有 NetworkEvents 能力的版本中注册于 Movement → General。仅使用原生 C++ 和现有游戏网络适配层。

参考 Naven Modern 的 LongJump 入站缓存顺序，按需求改为模块开关控制：**开启缓存，关闭全部释放**。不包含火球、跳跃、转头、Mouse5 或分段释放操作。

- 开启后等待第一份发给本玩家的实体速度包。仅匹配实体 ID，不要求向上或水平速度非零。
- 从这份包开始缓存后续入站包，包括其他实体移动、爆炸和事务包；出站包不处理。
- 正常开启期间不会定时或逐次释放。关闭模块时，按原始先后顺序回放所有缓存包。通用 Bind 可以用于切换模块开关。
- 缓存期间收到玩家位置纠正包，也按顺序入队，等待关闭模块时回放；不再采用 Naven 收到纠正包就关闭的处理。
- ArrayList 后缀显示缓存的自身击退次数，例如 `3 KB`；等待阶段显示 Waiting。

不设置击退次数、包数量或持有时长上限。其他模块已经取消的包不重复缓存，也不触发已有队列释放。实际内存分配失败时才结束缓存并回放已有包，避免引用失败导致静默丢包。

原生生命周期适配：断线、重生、玩家或世界对象变化时丢弃旧队列并释放 JNI 引用，防止回放到新世界。

回放不重新发布 PacketEvent，与参考的直接 `packet.handle(connection)` 一致。进入游戏包处理时不持有模块互斥锁，期间新到达的包仍进入同一 FIFO。

运行 `Client/test/movement/knockbackbalancer/run.ps1` 会在模拟 JNI/游戏环境中编译实际模块实现和事件总线，检查连续累计 10000 次 KB / 20000 包且不自动释放、关闭后完整有序释放、重新开启、纠正包、断线/重生、世界身份、其他模块取消、回放期间入包、引用失败和资源清理。真实服务器行为由用户注入验证。
