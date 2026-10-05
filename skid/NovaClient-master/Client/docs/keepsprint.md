# KeepSprint

参考：`skid/Rise-6.9.5-main/src/main/java/com/alan/clients/module/impl/ghost/KeepSprint.java`。仅保留独立 KeepSprint 模块；KillAura 内的 Keep sprint 开关及其专用攻击路径已删除。

## 独立模块

ClickGUI → Combat → KeepSprint，小齿轮内设置。模块默认关闭。

| 设置 | 默认 | 范围 / 步进 |
| --- | --- | --- |
| Hit Slow Down During Velocity | 0.6 | 0–1 / 0.05 |
| Hit Slow Down Normal | 0.6 | 0–1 / 0.05 |
| Buffer Decrease | 1 | 0.1–10 / 0.1 |
| Max Buffer | 5 | 1–10 / 1 |
| Velocity Hit Sprint | false | 开关 |
| Normal Hit Sprint | false | 开关 |
| Buffer Abuse | false | 开关 |
| Only In Air | false | 开关 |
| Rotation | false | 开关 |

Buffer 两个数字仅在 Buffer Abuse 开启时显示。减速数值是**命中后保留的水平速度比例**：0.6 保留 60%，1 保留全部。默认设置与 Rise 一样，仍产生原版减速；开启模块本身不等于取消减速。

原生 `State::Hit` 保留 Rise 的分支顺序：Only In Air 地面跳过；Buffer 累加、回落、归零过渡；然后以 `hurtTime > 0` 选择受伤参数。回落阶段保持原版 0.6 / false。关闭、重开模块不清空 combo/resetting；关闭 Buffer 只归零 combo，不额外重置 resetting。

Rotation 是独立的移动辅助开关，不修改上述攻击处理和参数。它使用现有 RotationManager 静默朝向移动方向，RotationBridge 随后修正输入：向后、侧向和斜向移动均转换为朝身体前方移动，实际行走方向和玩家视角保持原样。通过原版疾跑输入请求疾跑，不直接写速度，也不绕过原版饥饿、使用物品等限制。高版本同步 Input 记录的 sprint 位，1.7/1.8/1.20 使用已有疾跑按键适配。

其他模块占用转头、Scaffold 开启、暂停移动、潜行、打开界面、停止移动或 FreeCam 工作时让出控制。关闭 Rotation 或 KeepSprint 后，只释放自身的转头并恢复物理疾跑键；Sprint 模块开启时保留其自动疾跑请求。

Lunar 在原版攻击的 motionX 写入处派发一次事件，motionZ 和 setSprinting 消费同一次结果。1.20.1 使用 Player.attack 内的 setDeltaMovement / setSprinting；26.3 使用拆分后的 causeExtraKnockback。均只影响本地玩家，Y 速度不变。保留已有 Leader Prediction 缓冲期间对攻击减速写入的抑制。

## 验证与边界

运行 `Client/test/combat/keepsprint/run.ps1`，无需注入，不生成或编译 Java 源码：

- 从本地 Rise 监听器机械转换生成 C++ 参考函数，进行 200,000 次状态对比，包括设置切换、禁用、地面、受伤、分数减量和 Buffer 过渡。
- 编译实际模块代码，验证全部设置的默认值、范围、步进、可见性、数值显示、配置保存/恢复及事件接线。
- 编译 Lunar 钩子和公共适配器，检查一次攻击仅派发一次事件、XYZ、疾跑、远端实体和 Prediction 优先级。
- 在本机实际 Lunar 缓存和 1.20.1 / 26.3 class 文件中验证 JNI 成员，验证三个 Player 类的命中减速重定向，并检查三个控制器类不再包含 Aura 专用攻击重定向。
- 相关回归：AutoBlock 状态机、Hypixel、Prediction、KillAura CPS。

以上证明已覆盖的源码逻辑和离线路径一致，不等于服务器实测“100% 同步”。KillAura 和 AutoBlock 使用原有攻击路径，独立 KeepSprint 开启后仍会作用于正常命中产生的减速事件。实际注入、移动与服务器反馈由用户测试。
