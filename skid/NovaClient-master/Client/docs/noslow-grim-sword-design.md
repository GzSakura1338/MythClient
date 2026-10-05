# Lunar 1.8.9 剑格挡运行逻辑与 NoSlow 设计

分析日期：2026-09-23。范围仅限剑；食物、药水、弓不参与。本文是分析和设计，没有实现或实服通过结论。

后续仓库核对与可执行规格见 [完整实现文档](noslow-grim-sword-implementation.md)。已确认现有 LivingUpdateHook 位于父类运动计算之前，可与 InputHook 配合实现本地步行输入恢复；疾跑与 Grim 协议策略仍需分别闭合。

## 分析对象与证据边界

- Recaf MCP 当前主资源：`C:/Users/13366/.lunarclient/offline/multiver/cache/72c13d03/4b04e698/bake.zip`，19,946 个类。该路径来自 Lunar 1.8 启动日志 `profiles/1.8/logs/ichor-boot.log`。这套安装把完整处理后游戏类保存为 bake.zip；不能拿只有 Lunar 自身代码的 lunar.jar 代替它。
- 本文直接反编译这个完整归档中的 Minecraft 游戏类，并用 `EntityPlayerSP.onLivingUpdate` 字节码复核减速乘法。没有把临时导出的单个 EntityPlayerSP.class 当成完整游戏。
- 这是 Lunar 处理后的 1.8.9 游戏代码，含 Lunar/Apollo 注入，不能称为未经修改的 Mojang 原始 JAR。下述原有游戏调用链在该归档可见；Lunar 包装回调对全部配置组合的影响尚未穷尽。
- Grim 对象：`D:/AI/Minecraft/Nova/skid/Grim-2.0`。`build.gradle.kts:17` 的 baseVersion 为 2.3.74；这不证明目标服务器实际部署了同一源码、配置和协议转换环境。

## 游戏的实际调用链

1. `Minecraft.runTick` 处理按键，之后才调用 `theWorld.updateEntities()`。尚未使用物品时，右键进入 `rightClickMouse`；实体/方块交互会先处理，未消费交互才落到一般物品使用。
2. `PlayerControllerMP.sendUseItem` 先 `syncCurrentPlayItem`，再发送 C08 使用物品包，然后调用物品的 `useItemRightClick`。其返回值与物品堆变化有关，返回 false 不能直接解释为剑格挡失败。
3. `ItemSword.onItemRightClick` 调用玩家 `setItemInUse`。后者保存 itemInUse 和持续时间；剑的 `getItemUseAction` 返回 BLOCK。`EntityPlayer.isBlocking` 同时要求正在使用物品与 BLOCK 动作。
4. `EntityPlayer.onUpdate` 检查当前物品与 itemInUse 的对象是否相同；不同则清除使用状态，相同则更新使用计时。这不是仅检查“当前仍是一把剑”。
5. `EntityPlayerSP.onLivingUpdate` 先调用 `movementInput.updatePlayerMoveState()`，随后在 `isUsingItem() && !isRiding()` 时把 moveStrafe、moveForward 分别乘以 0.2，并清零 sprintToggleTimer。字节码明确包含两组 LDC 0.2F / FMUL / PUTFIELD。
6. 同一方法后续启动疾跑条件还要求 `!isUsingItem()`，且已有疾跑会受前进输入阈值等条件影响。因此消除 0.2 输入倍率与恢复正常疾跑是两个需要分别验证的问题。0.2 是输入倍率，不是所有地形/惯性条件下实测位移都恰为正常值的 20%。
7. 之后调用父类 `onLivingUpdate` 执行运动。`EntityPlayerSP.onUpdate` 在父类更新返回后调用 `onUpdateWalkingPlayer` 发送行走更新（骑乘有另一分支）。
8. 松开右键时，`Minecraft.runTick` 调用控制器 `onStoppedUsingItem`：同步槽位、发送 C07 RELEASE_USE_ITEM，再停止本地使用。

## 服务端格挡不能由动画证明

同一归档包含的原版服务端逻辑也经 Recaf 核对：

- `NetHandlerPlayServer.processPlayerBlockPlacement` 的使用物品分支进入 `ItemInWorldManager.tryUseItem`，再调用物品右键逻辑。
- `processPlayerDigging` 收到 RELEASE_USE_ITEM 时执行服务端玩家 `stopUsingItem()`。
- `EntityPlayer.damageEntity` 对可格挡的正伤害，在 isBlocking 成立时先计算 `(1 + damage) * 0.5`，之后还会处理护甲等。

因此“客户端保持举剑动画”“客户端移动不减速”“服务端持续承认格挡减伤”是三个不同指标。上述服务端代码说明原版语义，不能替代实际 Bukkit/Paper/Via 等目标环境的验证。

## 本地 Grim 的对应模型

以下路径均相对 `skid/Grim-2.0/common/src/main/java/ac/grim/grimac/`。

| 位置 | 已确认行为 | 设计含义 |
| --- | --- | --- |
| `utils/item/ItemBehaviourRegistry.java`、`LegacyItem.java` | 旧客户端使用 LegacyItem；1.8 剑被视为可使用物品 | Grim 没有忽略剑格挡 |
| `events/packets/PacketPlayerDigging.java:36` | 有效使用包会设置 slowedByUsingItem，记录事务和使用手 | 仅取消客户端减速不会同步改变 Grim 模型 |
| 同文件 `:65` | RELEASE_USE_ITEM 清除减速状态 | 也必须考虑服务端真实格挡被释放 |
| 同文件 `:80` 起 | 移动包/客户端 tick 结束时按使用槽与当前槽检查切换；持物变更另有条件分支 | 同槽 C09、切出再切回不能直接等同于稳定豁免 |
| `predictionengine/predictions/input/impl/FloatInputTransformer.java:25` | 使用状态下两轴乘 0.2，之后乘 0.98 | 与客户端输入减速相对应 |
| `predictionengine/predictions/PredictionEngine.java:155` | 使用状态下，把未翻转使用状态的候选误差交给 NoSlow | 存在不减速预测候选，不代表 NoSlow 自动放行 |
| 同文件约 `:763` 起 | 枚举使用状态并标记翻转候选 | 不应把预测容错当成长期许可 |
| `checks/impl/movement/NoSlow.java` | 默认误差阈值 0.001；相邻受检预测的异常状态影响 flagWithSetback | 单次没告警不足以证明成功 |

NoSlow 的 1.8 槽位变更分支会清除 didSlotChangeLastTick 和 flaggedLastTick，但没有直接 return；后面的误差检查仍执行。其注释不能被解读为所有首 tick/所有切槽永久免检。

## 建议的原生实现结构

遵守项目 C++/JNI 约束，不新增 Java 源码或 Java helper 字节码。

1. **严格剑门控**：模块启用、本地玩家有效、实际使用物品确认为剑且动作为 BLOCK、非骑乘时才参与。仅持剑但未开始使用时不干预。食品、药水、弓走原路径。
2. **独立减速钩子**：在上述两个输入乘法所在的准确阶段控制倍率，保留原始输入、潜行等其他约束。现有 `Client/src/minecraft/compat/lunar189/LunarRuntime.cpp:2906` 的 InputHook 在 updatePlayerMoveState 返回时派发 MoveInputEvent，此时原方法尚未执行 0.2；仅写回正常输入会再次被减速。不能在整段 onLivingUpdate 返回后才恢复输入，因为本 tick 运动已执行。
3. **实现前确认钩子能力**：通过现有原生变换/钩子设施寻找这个阶段的可控位置，按方法描述符和指令上下文校验；匹配失败则禁用该能力。当前分析没有实现或证明这个专用钩子已存在。不要全局伪造 isUsingItem，也不要把 motion 整体乘 5。
4. **疾跑单独处理**：先定义“100%”的对照基线为相同条件下普通步行；如果还要求正常疾跑，另行处理并验证启动/维持逻辑及服务端疾跑状态，不能把两项混成一个通过结果。
5. **统一使用状态所有权**：本地使用、画面格挡、已发送使用/释放记录分别保存。发送记录不叫“服务端确认”。与 AutoBlock 的包延迟同步以及 Scaffold 槽位操作协调，同一阶段只允许一个模块控制使用/释放/持物操作。
6. **生命周期收尾**：松键、换物品、死亡、切世界、断线、模块关闭、JNI 异常都撤销本模块临时控制；不要用陈旧物品引用恢复格挡。

## Grim 模式的研究结论与下一步

已确认可定位客户端的减速原因；尚未确认“连续服务端真实格挡 + 全速移动 + 该 Grim 不告警”的方案。直接删除减速倍率只完成客户端一侧。

可验证的研究方向是使用状态变化与运动预测采样之间的时序，以及旧客户端槽位变化的兼容处理。但必须同时观察 Grim 状态和服务端实际使用状态，不能凭源码中的一个布尔重置就宣布绕过。发送释放会结束原版真实格挡；使用动画掩盖释放不满足连续防御指标。

建议先做原生只读观测，再决定协议策略：逐 tick 记录输入更新前后值、使用物品身份、实际位移、疾跑状态、槽位与使用/释放/移动包的发送顺序；在可控目标服对齐 Grim 预测误差、slowed 状态和真实格挡伤害。当前未给出经验证的发包序列。

验收矩阵：

- 普通步行与疾跑分别建立未格挡/原版格挡/实验模式对照；比较稳定阶段位移，不只看速度显示。
- 直行、斜行、跳跃、潜行、碰撞、不同延迟和 tick 跳过分别测试。
- 食物、药水、弓不得改变使用速度或消耗行为。
- 松键、剑换剑、剑换食物、AutoBlock 开关、Scaffold 换槽、重连后状态应正确。
- 实际受击时对比服务端格挡减伤，覆盖状态切换附近的攻击时刻；只看到举剑不算通过。
- 固定并记录服务端版本、Grim 构建/配置和 Via 链路；检查相关动作顺序检测，不只观察 NoSlow 单项。

只有位移、Grim 检测和真实防御三个指标都验证后，才可以把结果称为该环境下的全速剑格挡。
