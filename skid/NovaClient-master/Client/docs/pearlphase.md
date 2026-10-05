# PearlPhase

入口：Minecraft → Player → PearlPhase。独立于 AutoPearl。

- 默认开启执行一次，自动切到珍珠、静默投掷、切回，然后关闭模块。
- `Always` 默认关闭；开启后只在着地且水平碰撞时重复触发，至少间隔一秒，并遵守原版物品冷却。
- `Inventory` 默认开启，高版本可从背包取珍珠；优先快捷栏。背包临时交换在使用后恢复，快捷栏选择在下一 tick 恢复。1.7/1.8 只使用快捷栏，隐藏该选项。
- Scaffold、AutoMLG、AutoCharge、AutoMace、AutoRod、AutoBlock 或背包操作占用交互时等待；单次请求等待超过 60 tick 取消，避免过很久后突然投掷。

## 参考与范围

参考 `skid/OpenMio-main/src/main/java/me/mioclient/module/exploit/Phase.java` 的 Pearl 投掷分支、`getFloatArray1759`、`getDoubleArray1760`，以及对应的方向、投影与背包 helper。

方向计算保留原公式：无输入时按向前处理；前后斜移归一化；投影至当前方块边缘；反向朝方块中心的角度加 180°；使用 Java 的取整规则。正常高度俯角 85°，Y 小数部分超过 0.1 时为 81°，梯子为 80°。环境检查的 Y 使用四舍五入，方向计算使用向下取整，二者按原实现区分。

本次移植的是珍珠入块功能，不含 Phase 的 Clip 位置移动，也未接入其可选的攻击清障、提前放黑曜石与 NCP 预交互。Mio 的设置初始化委托给 native Loader，源码不能确认默认值；上述默认值是 Nova 明确选择的值。

适配差异：Nova 使用现有碰撞形状接口判断脚部/头部占用，以低矮碰撞箱检测上升游泳/爬行姿态；Mio 使用 block 的 collidable 属性和 swimming pose。持续模式采用固定一秒最低间隔，未复制 Mio 的 TPS 修正。特殊无碰撞方块、爬行/飞行姿态和低 TPS 条件不宣称逐状态等价。

## 接入与生命周期

实现全部是 C++/JNI。`GamePearlPhase` 复用已有版本映射读取输入和环境；`PearlAim` 保存方向公式；`PearlPhase.cpp` 负责触发、使用和恢复。

高版本使用 RotationManager 的独占转头、移动修复与独占物品使用事务，角度在物品使用及移动提交之间保持一致，并阻止同 tick 后续攻击；下一 tick 释放。旧版本在投掷前提交方向。没有复制 Mio 在投掷之后额外发送 Full movement 的分支。

单次使用失败不会自动重复投掷。关闭模块仍执行下一 tick 的槽位恢复；手动滚轮切槽保留用户选择；换世界/重生取消旧世界操作。背包恢复前核对暂存原物品及手中珍珠，发现物品被修改就停止回换；持续失败超时会提示，避免无限占用。

## 验证

`Client/test/player/pearlphase/run.ps1` 编译生产模块和 JNI/event 替身，检查角度、跨零点坐标、背包回换、使用/同步失败、手动滚轮、切世界、持续触发条件、其他模块占用和旧版先转头后使用顺序。

`Client/test/utils/rotation/run.ps1` 检查共享转头连续性、移动修复、物品与攻击互斥及 tick 边界。DLL 构建验证不能代替游戏内测试；未据此保证服务器接受珍珠入块。
