# Speed

ClickGUI → Movement → Speed。只有模块开关和绑定键，默认关闭。

移动时通过原版输入请求疾跑，在地面清除跳跃冷却并自动跳跃。起跳 tick 静默朝向实际移动方向；空中偏转 45°，由公共 RotationBridge 将输入转换为斜向，保持玩家视角和原本行走方向。保留原版跳跃力度、速度属性、碰撞与疾跑资格判定，模块不修改速度或 Timer。

停止移动、潜行、打开界面、FreeCam、Scaffold、背包操作暂停移动、使用物品、水/岩浆、攀爬、乘坐、飞行或滑翔时暂停。KillAura 普通瞄准期间继续落地自动跳跃，45° 转头让位给 KillAura；此时不额外请求疾跑，保留暴击取消疾跑和原有移动修复。其他转头模式、最高优先级请求或独占放置时暂停。Speed 开启时 KeepSprint 的 Rotation 让位；KeepSprint 原有攻击减速与 Buffer 处理保持不变。关闭后只释放自身转头并恢复疾跑按键。

## Recaf 分析

- Lunar 1.8.9：`cache/72c13d03/d3098b28/bake.zip`。`EntityLivingBase.onLivingUpdate()` 在移动前将输入乘 0.98；`Entity.moveFlying(FFF)` 仅在输入长度超过 1 时归一化，因此直线输入长度为 0.98，斜向输入归一化为 1，斜向的加速度约增加 2.04%。这不是最终移动速度提升 2.04% 的保证。
- `EntityLivingBase.jump()` 在疾跑时沿 yaw 添加 0.2 水平推力；偏转 45° 后正向分量只有约 0.1414，因此地面起跳保持直线，空中才偏转。
- Lunar 1.21.11：`cache/d1937f9/cc81b79c/bake.zip`。`KeyboardInput.tick()`（`class_743.method_3129`）已经归一化二维输入；`LocalPlayer.modifyInput()`（`class_746.method_67270`）随后乘 0.98。单纯偏转 45° 不增加高版本加速度，主要收益来自自动疾跑跳跃。疾跑跳跃水平推力仍为 0.2（`class_1309.method_6043`）。

上述源码与运行时输入模拟用于验证机制，不代替实际服务器表现测试。
