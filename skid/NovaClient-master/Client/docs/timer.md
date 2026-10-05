# Timer

位置：Movement → General → Timer。支持 Lunar 1.8.9、1.7.10 和
EaseCation（NeoForge / Minecraft 1.21.8）。旧版使用 GameTimer / TimerSpeed 映射。
EC 在 Minecraft.getTickTargetMillis(F)F 的所有返回路径上按倍率缩放游戏 tick 周期，
保留原版／服务器计算出的基础周期，不修改 UI 实时时钟或 final 字段。
没有玩家／世界及卸载时恢复正常周期；不生成 Java 源文件。

参考：skid/Rise-6.9.5-main/src/main/java/com/alan/clients/module/impl/other/Timer.java。
只移植普通倍率和 Compensate Low Timer，不含 Watchdog 分支。

- Min speed / Max speed：0.1–20，默认 1 / 2，每次 Motion Pre 在区间内随机。
- Compensate low timer：默认关闭。小齿轮内包含 Compensate speed
  （1–5，默认 1.5）和 Balance multiplier（0.1–10，默认 1）。
- 开启补偿后，最小倍率或当前倍率低于 1 时，每个未取消的出站移动包累计
  Balance multiplier 点额度。这是 Rise 的按包计数语义，不是实际节省时间。
- 模块开启、有额度时，再按绑定键进入补偿；补偿中再次按键不会关闭模块。
  每个未取消移动包消耗 1 点，耗尽立即恢复 1 倍，下一次模块更新关闭开关。
- ClickGUI 直接关闭、卸载会恢复 1 倍；离开世界会清空额度并恢复正常倍率。
- Nova 没有 Rise 的 PacketQueueComponent.cR 标志，因此不会把已取消的
  事件计入额度；模块本身不拦截、缓存或回放任何包。

测试：`Client/test/movement/timer/run.ps1`。
