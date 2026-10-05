# Starain Scaffold 移植

参考 `skid/Starain-1.20.1-main/src/main/java/today/starain/module/impl/player/Scaffold.java`。
实现位于 `src/dev/myth/model/player/Scaffold.java`，列在 Player，默认关闭、未绑定。
按用户要求移除全部 ESP，以及卡空、秒放方块自救、紧急递归补放和跳帧逻辑。

| 设置 | 默认 | 范围或选项 |
| --- | --- | --- |
| Telly | false | 开关 |
| Snap | false | 非 Telly 可见 |
| Snap Dist | 0.1 | 0～1，步进 0.01，Snap 可见 |
| 45 Degrees | false | 开关 |
| Swing | true | 开关 |
| RotationMode | Rise | Rise / Heypixel |
| RayCast | Normal | Normal / Strict |
| RotSpeed | 127 | 1～180 |
| RotSpeed2 | 36 | 1～180，Heypixel 可见 |
| BackSpeed | 180 | 1～180，Telly 可见 |
| TellyTicks | 1 | 0～6，Telly 可见 |
| SafeWalk | false | 非 Telly 可见 |
| Edge Dist | 0.1 | 0～1，步进 0.01，SafeWalk 且非 Telly 可见 |

保留源方块黑名单、排除半砖/植物/提示物品、首个有效热栏槽位选择、
4.5 格范围内逐层搜索可点击支撑方块、避开有菜单方块、射线条件和随机点击点。
Telly 在地面移动时自动跳跃，保留建桥高度；按空中 tick 切换 Heypixel 速度。
Snap 和 SafeWalk 使用原方块坐标边缘距离算法。
候选 yaw 的排序、45 Degrees 排除和 pitch 搜索沿用 Starain。

## 26.2 接入

- 使用现有 `Setting` / `Model` / 配置 / 按键 / 通知系统；后缀由 `getInfo()` 返回 Normal、Snap、Telly。
- 普通转头调用共享 `RotationManager.setRotations`，输入修正继续使用现有 MoveFix，
  不改动已修复的 Nova yaw 连续性工具。
- 热栏 `selected/getSelected` 改为 `getSelectedSlot/setSelectedSlot/getSelectedItem`。
- 原 BushBlock 在新版本拆分为 VegetationBlock，FungusBlock 改名 NetherFungusBlock。
- 放置仅通过普通 Normal / Telly 路径，遵守射线及 TellyTicks 条件，
  `useItemOn` 走原版预测与序号流程；不额外发送强制转头包或跳过玩家 tick。
- 缺少玩家/世界时安全清理，打开界面暂停放置；关闭恢复原槽位。
- 按 Starain 的联动条件，Scaffold 开启时暂停 KillAura；同时清除旧攻击队列避免延迟攻击。

`scripts/verify-starain-scaffold.ps1` 逐方法全文比较 10 个保留的主要算法，
仅允许上述包/接口/随机助手替换。
生命周期、框架接入及用户要求删除的功能不参与全文一致声明。

验证代码位于 `build/killaura-validation`，不会包含在正式 JAR。
验证设置、筛选、切槽与恢复、邻居支撑搜索、Normal / Strict 放置、
SafeWalk、Snap、Telly、Swing、KillAura 暂停、正常玩家 tick、重生清理和配置保存；
最终通过集成服务端确认实际放置方块，而非只检查客户端预测。
