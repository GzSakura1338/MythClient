# AutoMace：LiquidBounce 参考重构

## 参考与适配范围

参考 `skid/LiquidBounce-nextgen/src/main/kotlin/net/ccbluex/liquidbounce/features/module/modules/combat/ModuleAutoWeapon.kt` 的 `AutoMace`、`ChangeOn.OnAttack`、`ChangeOn.OnTarget`，以及 `KillAuraClicker` 的重锤重击冷却例外。

LiquidBounce 的 AutoMace 本质上是武器选择功能，由其他攻击来源触发；Nova 保留独立选敌、静默瞄准、自动攻击。因此这里是原生 C++/JNI 适配，不是完全相同的模块：

- Quick Switch：满足重击条件且实际射线命中后，在攻击前切锤并同步槽位。
- Quick Switch 专属 `Axe` 开关（默认关闭）：目标正在有效举盾、快捷栏有可触及目标的斧头时，先切斧并攻击一次，下一游戏 tick 重新检查下落状态、目标和射线后切锤攻击；再下一 tick 恢复最初槽位。不等待服务端盾牌动画消失，也不把客户端提交当作破盾成功。无盾、无斧或斧头无法触及时保持直接切锤。
- Normal：下降时发现当前可命中的目标就准备转头、提前持锤；满足重击条件再攻击。
- 不强制等满攻击冷却，避免错过下落命中窗口。低冷却仍影响原版基础伤害。
- 使用物理槽位所有权，攻击后的下一游戏 tick 恢复，不使用 LiquidBounce 的默认 20 tick 静默持槽。
- 多把重锤选择快捷栏左起第一把；没有移植其附魔评分算法。
- 不移植 `ModuleMaceKill` 的位置包逻辑，不修改下落距离、位置或 onGround。

支持 Lunar 1.21.11、Minecraft 26.3；无原版重锤的旧版本不执行。沿用各版本集中 mapping，不新增 Java 源码或 Java helper。

## 原版依据

先前在 Lunar `bake.zip` 的反编译分析：`class_9362.method_58659` 的重击条件为 `fallDistance > 1.5 && !isFallFlying()`。模块另外要求真实下降、未使用物品/挖掘，避开水、岩浆、攀爬、载具与飞行状态。这些额外条件属于模块策略。

重击附加伤害按实际下落距离 d 分段计算：d≤3 为 4d，3<d≤8 为 12+2(d−3)，d>8 为 22+(d−8)。护甲、附魔、免伤和服务端规则仍参与结算。客户端提交攻击不等于服务端命中成功。

范围读取当前物品的 `ATTACK_RANGE`，没有组件时使用 `AttackRange.defaultFor(player)`；最终同时验证配置 Range、原版 `isInRange` 和实际实体射线。Lunar 组件读取的映射 owner 使用 `DataComponentHolder`，不能用不存在的 ItemStack 同名映射代替。

## 状态和时序

```text
Idle → Aiming → Restore（下一 tick）→ Idle
            ↘ 目标失效/槽位变化/超时 → 释放 → MLG 可接管
```

GameTick Pre 负责选敌、转头和普通模式切槽。PrePlayerInteract 是原生 `handleKeybinds` 入口，每游戏 tick 执行，负责唯一的攻击提交；它不是仅在右键按下时触发。

攻击链路必须为 `AcquireAttack → BeginAttackSubmission → AttackAndSwing → EndAttackSubmission`。旧实现缺少 AcquireAttack，导致始终无法提交。共享攻击许可同时防止其他攻击或右键交互插入这一周期。现代控制器 guard 通过后才通知攻击观察者，被拦截的调用不产生假攻击事件。

选敌使用当前 tick 眼睛位置与实体包围盒，先验证预期瞄准点可见；提交前再检查平滑、灵敏度量化后的实际 RotationManager 角度。复用连续 yaw 和移动修复。瞄准最多持续 3 个游戏 tick，错过窗口就归还控制，不强制跳角。

每次下降最多提交一次重锤攻击，Axe 分支最多额外提交一次前置斧头攻击；遵守每 tick 一次原生攻击许可。斧头攻击后只保留下一 tick 的重锤接续窗口，落地、目标失效、切换模式或交互冲突时释放给 MLG，不循环切斧。客户端方法报错也不能假定包未发送，因此提交后同样等待下一 tick 清理，不立即重发。真实落地或下落距离重置后重新允许下一轮。取消已取得的瞄准窗口后，本轮下降不反复抢回控制。

举盾检测使用原版 `LivingEntity.getItemBlockingWith()` 返回的有效格挡物品；Lunar 1.21.11 的对应方法 `class_1309.method_62821` 经 Recaf 确认同时检查正在使用物品、格挡组件和举盾延迟。斧头使用 `ItemTags.AXES` 标签识别，不依赖显示名称或 26.3 已移除的 AxeItem 类。实际破盾与重锤命中仍由服务器处理，极短的剩余下落窗口可能来不及完成两次攻击。

恢复槽位前检查同一玩家、同一世界、选中槽未被用户改变、原物品和锤的 ItemStack 身份未变化。关闭模块只请求清理，由游戏线程处理；切世界不向新玩家强行恢复旧槽位。

## AutoMace 与 AutoMLG 优先级

用户要求：**先锤，锤不到再 MLG；MLG 不能打断正在工作的重锤。**

- AutoMace 的规划与攻击事件先于 AutoMLG。
- 无锤、无合法目标、不可达或遮挡时不取得控制，MLG 保持原流程。
- 确认可命中的目标后，AutoMLG 释放准备/收水阶段的转头、槽位所有权。
- AutoMace 工作期间，MLG 的 tick 和交互提交都暂停，不切水桶、不覆盖转头。
- 重锤目标失效、瞄准超时或提交后的下一 tick，归还控制；MLG 不额外等待原来的 250ms 冷却。
- 已提交的桶操作不能撤回；本 tick 不再插入重锤攻击，等待下一个合法交互周期。
- 攻击提交后的同 tick 不切回武器或放水。是否免摔以原版状态为准，攻击提交不是“已经安全”的证明。

AutoMace 使用时 KillAura 暂停自己的选敌/攻击与暴击疾跑控制，InvManager 暂停整理；AutoBlock、AutoRod、AutoCharge、Refill、Scaffold、FreeCam/FreeLook，以及延迟中的 AntiKB/Velocity/Backtrack 继续遵守现有互斥条件。

## 设置

Mode 默认 Normal；Range 默认 3；FOV 默认 180、最大 360；Rotation speed 为每 tick 最大角步长，默认 180。Target 齿轮保留 Player/Mobs/Animals，默认只开 Player，测试生物时须开启相应类型。Teams 与 AntiBot 继续统一过滤玩家。

## 验证

- `test/combat/automace/run.ps1`：将生产 AutoMace 放入原生运行夹具，验证实际进入模拟原生攻击入口、两种切槽时机、低冷却重击、每次下降一次、慢转头失败释放、MLG 优先级、用户切槽、换世界、关闭、目标类型和交互冲突。
- `test/player/automlg/run.ps1`：生产 AutoMLG 既有放水/收水/灭火/蛛网测试，增加重锤抢占、立即恢复和已提交用桶保护；同时验证 Forge 字节码 hook 结构。
- `test/utils/rotation/run.ps1`：生产 RotationManager 的连续 yaw、手部插值、共享交互，以及攻击所有权、单次原生入口、发移动后禁止补攻击。
- `test/combat/automace/mapping.ps1`：从现有 Lunar bake 与 26.3 jar 读取原始类，核对生产 mapping 的方法签名，审计原生攻击 guard 改写后的分支、栈映射和异常表；不修改原始 jar。

离线测试和编译不能替代实机命中、服务端判定验证。DLL 输出 `Client/build-vs2026/Release/NovaClientBase.dll`，由用户自行注入。
