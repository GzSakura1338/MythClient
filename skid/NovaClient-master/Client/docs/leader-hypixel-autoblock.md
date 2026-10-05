# Leader Lite B0.4 Hypixel AutoBlock 原生移植

参考文件：`skid/Leader Lite-B0.4.jar`，SHA256 为
`47194DCCF7950451F23B8542E3AE8356F32A1A51FE37140DB064EFB243BBBBC8`。
使用 Recaf 读取 `leader.module.modules.combat.KillAura`、`BlinkManager`、
`PlayerStateManager`、`MixinEntityPlayerSP`、`MixinNetworkManager` 及相关属性和工具方法。
项目实现及测试均为 C++/Python，没有新增 Java 源码。

## 入口

KillAura → AutoBlock 小齿轮 → Mode = Hypixel。独立 AutoBlock 使用同一实现。
默认 HypixelMode 为 OldHypixel，LagMode 为 3Tick，与参考文件一致。

- HypixelMode：OldHypixel / Without NoSlow / Custom / Lag。
- LagMode：2Tick / 3Tick / 4Tick / 3Tick + 2Tick / 5Tick。
- 各模式参数只在对应模式显示，原 Lag/Vanilla/Grim 的配置索引保持不变。
- Hypixel 用自身的 AutoBlock Aps 控制格挡周期中的攻击，不叠加 Nova 原有 Hit select 和帧计时 CPS。

## 代码对应

| Leader | Nova |
|---|---|
| KillAura.onUpdate 的 Hypixel 分支 | `src/minecraft/modules/combat/autoblock/HypixelAutoBlock.h` |
| UpdateEvent PRE/POST | EntityPlayerSP.onUpdate HEAD/各正常 RETURN；取消的 tick 不触发 POST |
| startBlock / stopBlock / interactAttack | `src/minecraft/compat/lunar189/LunarRuntime.cpp` 的 Direct 操作和 InteractSwordBlock |
| BlinkManager.offerPacket / setBlinkState | 原有原生发包队列新增 Hypixel 策略 |
| PlayerStateManager.digging / placing | 发包进入队列前更新；每个 C03 清空 |
| 原选人及旋转到实际攻击的衔接 | `src/minecraft/modules/combat/autoblock/AutoBlockHypixel.cpp`、KillAura.ReadHypixelTarget |
| 属性定义 | `src/minecraft/modules/combat/autoblock/AutoBlockModuleBase.h`，两个 UI 入口共用 |

C07、C08 和 C09 直接发送并同步本地使用物品状态，不经过模拟按键延迟。
攻击成功后按 INTERACT_AT → INTERACT → C08 顺序重新格挡。
原版 C07 会立即把 digging 设为 true，状态机随后重新检查 Busy，因此同一移动包周期不一定会执行攻击。
Custom 保留 `blockTick + 1` 比较及 `MaxTick - 1` 回绕，没有擅自修正它的计数逻辑。
原版若干 tick 默认值为 0，虽属性验证器只接受 1–5，构造时不验证；Nova 明确保留 0 哨兵并允许保存、恢复。

Blink 对 keepalive 和聊天包放行，队列为空时放行 C0F，其余按顺序缓存/释放。
格挡 HUD 记录实际发出的包，视觉上的 AlwaysRenderBlocking 不会直接增加统计格挡率。

## 对齐边界

移植目标是 Hypixel AutoBlock 状态机，不是整个 Leader 客户端。
Nova 保留自己的目标类型、Teams、旋转选择、非格挡 CPS 和独立 AutoBlock 的手动攻击入口。
攻击射线使用 Nova 的最终静默旋转；格挡中使用 Leader 的 3.5 挥手距离、3.0 攻击距离、8.0 交互射线距离。
格挡目标搜索使用独立 AutoBlock Range，默认 6。

Leader 的 NoSlow、Disabler/C09 第二把剑选择、BlinkSettings 慢速释放、SmartAttack
仍按这些独立扩展关闭时的默认路径运行，使用默认切槽算法。
AntiKB 新增的 Prediction 已接入 `VelocityCanReduce`、停止格挡、取消攻击、ExtraAttack 阶段重置及共享 Blink 所有权。
具体参数、原版特殊控制流和必要的生命周期差异见 [Prediction 移植说明](leader-prediction-velocity.md)。

世界切换、断线、切换模式、打开菜单时释放队列、恢复槽位和输入所有权属于 Nova 集成清理。
这些保护行为不会被当作 Leader 原始控制流的一部分宣称完全相同。

目标快照、盒内射线、独立模块范围和改绑输入的适配修正见 [对齐复查](leader-prediction-audit.md)。

## 验证

`Client/test/combat/autoblock/run_hypixel.ps1`：核对本机 Lunar 缓存中的新增 JNI 绑定；
把真实 Minecraft / EntityPlayerSP 类文件交给生产字节码编辑器，并校验跳转、异常表、栈映射；
检查五种 Lag 包序列、OldHypixel、Without NoSlow、Custom POST、APS、目标丢失、切槽恢复，
以及 480 组持续状态机用例；检查生产属性的默认值、显隐和序列化。

`run_autoblock.ps1` 和 `run_killaura_cps.ps1` 覆盖原有模式和共享入口回归。
Release 构建附带反射加载器静态 TLS 检查。
上述属于离线验证；实际注入、服务器交互和视觉效果由用户测试，不据此承诺服务端效果。
