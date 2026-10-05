# AnchorAura

独立 Combat 模块 `AnchorAura`，不替换 `LegitAnchorAura`，配置分别保存。实现为原生 C++ / JNI，沿用 Nova RotationManager、移动修复、HotbarSession、交互事务和 ImGui 放置 ESP。

## Mio 对照

参考 `skid/OpenMio-main/src/main/java/me/mioclient/module/combat/AnchorAura.java`、`AnchorAuraData.java`、`AutoCraftMode.java`、`PhaseESPSearchHelper4_2.java` 和爆炸伤害辅助类。

| Mio 流程 | Nova 实现 |
| --- | --- |
| `getAnchorAuraData2872` 网格顺序 | x/y/z 嵌套；偏移从 `-floor(range)` 到 `<ceil(range)`；x/z 以玩家位置、y 以眼睛位置为基准；按方块中心限制距离 |
| 新位置比较 | 完成一次完整扫描后选择最大目标伤害；同伤害取后一个；首个有效已有锚优先 |
| `getAutoCrystalData_22873` | 比较全部符合条件的玩家，使用当前包围盒，不加移动预测；排除自己、队友、假人、死亡、旁观和创造玩家 |
| `getVec3d2871` | X8 内部角点（各轴 .05/.95）按距离排序，中心后备；提交时以真实射线复核交互面 |
| 支撑方块顺序 | 下、北、东、南、西、上 |
| 爆炸伤害 | 强度 5；包围盒曝光采样；忽略爆炸锚自身及抗爆性小于 600 的方块；沿用难度、护甲、韧性、抗性和保护减伤 |
| `is2874` | 最低目标伤害；非致死攻击的自身伤害必须小于 Max Self Damage；目标致死可豁免该上限，但始终禁止预估自杀 |
| 操作循环 | 放锚 → 观察锚出现 → 充能 → 观察电量 → 引爆 → 观察锚消失 → 原位置重新验证并继续 |
| 切物品 | 优先当前匹配物品，否则搜索快捷栏；支持副手锚；每次操作后恢复原槽位；尊重手动滚轮切换 |
| 与水晶协调 | AutoCrystal 伤害严格更高时优先；相等优先锚；已提交且待确认的事务优先完成 |
| 维度限制 | 锚不会爆炸的维度停止模块 |

## 简化设置

- Target：Range，默认 4.5；Through Walls，默认关闭。目标搜索固定 12 格，360°。
- Delay：默认 0 ms，步进 5 ms。每次交互之间的最小间隔。
- Silent：只隐藏切物品的手部效果；转头始终走 Nova 静默转头系统。
- Swing：控制手部挥动动画；关闭仍发送必要的挥手包。
- Rotation → Speed：默认 90；使用平滑步进和鼠标灵敏度量化。
- Safety：Min Damage 默认 4；Max Self Damage 默认 6。防自杀固定开启。
- Place ESP：复用现有 HUD 风格及动画。

Through Walls 开启时，遮挡交互范围与 Range 相同；关闭要求正常视线。不会凭空放锚，也不会将固体/液体单元冒充可放置位置。

## 接入与兼容边界

- 两个锚模块同时开启时，AnchorAura 优先，LegitAnchorAura 暂停；关闭新模块后旧模块可继续。
- 使用独占 RotationManager；`SetExclusiveRotations` 自动启用移动修复。未修改既有 AimModulo360 修复。
- 仅在 PrePlayerInteract 提交一次原生 `useItemOn`，由共享事务避免同 tick 的攻击/物品使用冲突；装备同步在使用前完成。
- DuplicateRotPlace 检查复用实际出站包的 PlacementWire 历史，不使用本 tick 拟发送角度代替上一个已发送角度。模块开关不清除该历史。
- 每次扫描最多处理 128 格、8 个目标伤害计算，2 ms 软预算；单次 JNI 查询不能中断。已选位置和待提交操作实时重新验证。
- 待观察状态变化最多等待 500 ms，超时对该位置冷却 500 ms。状态来自客户端世界更新，未声称这等同于独立服务端回执。
- 正在吃东西、挖掘、潜行、打开界面、延迟发包，或 Scaffold / AutoMLG / AutoMace / AutoCharge 等接管时暂停。
- 当前原生锚接口支持 Lunar 1.21.11 与 Minecraft 26.3；其他版本会提示不支持并关闭。

这是 Mio 选位/伤害/循环策略的原生适配，不是逐包、逐时序 100% 相同的实现：Mio 的同事件多操作、异步读取世界、网络线程立即补放、客户端删除锚（inhibit）及 air/liquid placement 没有照搬，改为 Nova 主线程预算扫描、单次交互和状态观察。Mio 的设置初始化在 native Loader 中，源码没有可核实的完整默认值；其保留位置检查还带有反编译错误标记，不能据此宣称所有分支逐字等价。

## 验证

`Client/test/combat/anchoraura/run.ps1` 编译实际生产模块，模拟 JNI、事件和世界状态，覆盖连续循环、主副手、切回、伤害排名、已有锚优先、防自杀/致死豁免、遮挡、目标过滤、移动量化转头、实际移动包顺序、未确认超时、换世界、两个锚模块共同注册及恢复。

另运行旧 LegitAnchorAura、AutoCrystal、LegitCrystalAura 和转头回归，以及 GameAnchor / GameCrystal 对两份实际游戏 JAR 的 JNI 映射校验。

这些验证不等同于在 Grim 服务器上的验收；具体服务器规则、网络延迟、Through Walls 行为及客户端预测结果仍需实服测试。
