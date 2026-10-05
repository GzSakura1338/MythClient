# AntiBot

入口：Misc → General → AntiBot。默认关闭，无模式、无额外设置，与 Starain 1.20.1 一致。

源文件：`skid/Starain-1.20.1-main/src/main/java/today/starain/module/impl/misc/AntiBot.java`。
判定为：模块开启，且实体 UUID 不在连接的 `getOnlinePlayerIds()` 集合中。
不检查名字、血量、隐身、实体年龄或皮肤；不维护额外黑白名单，不删除实体，不改动入站包，也没有事件计时器。

这里指连接维护的完整玩家 UUID 集合，不只是屏幕上可见的 Tab 玩家。隐藏在 Tab 界面但仍保留 PlayerInfo 的玩家不会被识别为 Bot。反之，仍保留实体、但 UUID 已从在线列表删除的目标会立即被过滤。

保留原版没有 `instanceof Player` 限制的行为：不在在线列表中的生物和 NPC 也会被过滤，即使 KillAura 开启了对应目标类型。进入世界时实体先于玩家列表到达，也会暂时被识别为 Bot；不额外添加等待期。断线、无效实体或 JNI 读取异常时返回“不作 Bot 判定”，避免将读取失败等同于空列表。

实现位置：

- [AntiBot 模块](../src/minecraft/modules/misc/antibot/AntiBot.cpp)：模块开关和统一 `antibot::IsBot` 入口。
- [玩家列表读取](../src/minecraft/java/game/GamePlayerList.cpp)：C++ JNI 实现；1.20.1 / 26.3 调用在线 UUID 集合的 `contains`，1.8.9 调用同一个 UUID 玩家信息映射的 `getPlayerInfo(UUID)`，以非空结果判断成员关系。
- 各版本 `.inc` 的 `NOVA_PLAYER_LIST_MAPPING` 段保存全部映射，JNI 全局引用随网络访问器卸载释放。

Starain 的 KillAura 选敌、Hit select 和 Velocity 自动攻击/前进条件已接入对应判定。Nova 的 AimAssist、共享 AutoBlock（含 Hypixel）以及 AntiKB Reduce/Prediction 自动攻击也复用统一入口，防止从其他自动攻击路径绕过过滤。不会拦截原版手动攻击。Nova 没有 AutoThrow 模块，未为此创建额外功能。

验证：`Client/test/misc/antibot/run.ps1` 测试实际生产判定；KillAura 和 AutoBlock 的现有测试补充 Bot 攻击过滤检查。游戏内由用户注入验证。
