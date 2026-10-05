# AutoBlock Lag New / APS

参考来源为 skid/AutoBlock.java 的 Expo LAG_NEW / 7APS 分支。采用原生 C++/JNI，不生成 Java 或辅助字节码。原有模式 0–4 不变，Lag New 为 5；独立 AutoBlock 与 KillAura 共用同一引擎，独立模块优先。

## 设置与范围

沿用 Range、Require right mouse、Force block animation。APS Mode 包含 7APS（默认）、3APS、5APS、10APS、14APS；7APS 保留原来的配置索引 0。Manual Left Click 默认关闭，开启后必须有真实点击或模块提交的攻击请求。未实现 LAG_NEW_PRE、Expo NoSlow、Smart Unblock 或其专属筛选器。

新增档位采用当前 Nova 修复版的独立移动区间规则，不是一比一复制 Expo 对应分支。低档位延长格挡，高档位在未格挡阶段每 tick 攻击一次，再进入格挡和解除阶段，不在同一 tick 突发多个攻击。档位变化从下一攻击阶段生效，不中断已有包组。

| 档位 | 连续攻击 tick | 格挡 tick | 解除 tick | 20 TPS 理论攻击次数/秒 |
| --- | --- | --- | --- | --- |
| 3APS | 1 | 4 | 1 | 3.33 |
| 5APS | 1 | 2 | 1 | 5 |
| 7APS | 1 | 1 | 1 | 6.67 |
| 10APS | 2 | 1 | 1 | 10 |
| 14APS | 5 | 1 | 1 | 14.29 |

保留参考档位名称，实际频率取决于整数 tick、有效射线、手动点击及模块协作。高档位降低格挡覆盖比例；此表不是服务器受伤次数或防砍率。

参考文件缺少 PacketManager、OutgoingPacketState、CombatUtil、AutoBlockBinder 等依赖，并保留未解析的动态调用。因此不能声称完整代码行为或服务器结果与 Expo 一致。以下时序是 Nova 的明确适配。

## 当前循环

所有攻击、格挡和解除格挡都在 PrePlayerInteract 输入阶段执行，本地使用状态必须在移动计算前确定。PostPlayerTick 只释放/重新申请缓存，不生成使用动作。

| 阶段 | 输入动作 | 本地移动计算 | 随后的移动包 |
| --- | --- | --- | --- |
| 3 → 2 → 7 | 挥手，射线命中时攻击 | 不因格挡减速 | C03 结束攻击区间 |
| 7 → 11 → 0 | 重新格挡 | 原版格挡减速 | C03 后释放缓存并重新申请 |
| 0 → 3 | 解除格挡 | 不因格挡减速 | C03 结束解除区间 |

首次未格挡时从阶段 1 进入攻击。上述表格描述默认 7APS 的三 tick 稳定周期，其他档位按档位表延长攻击/格挡阶段。不能据此推导任意服务器环境中的精确速度或格挡率。

输入动作后必须观察到真实生成的移动包才推进，避免跳过玩家更新时把攻击、格挡和解除压到同一区间。329ms 手动点击兜底只安排后续格挡，不生成额外攻击，不绕过移动边界。正常包序为 C0A → C02 ATTACK → C03 → C08 → C03 → C07 RELEASE → C03。

之前把格挡放在 PostPlayerTick 会出现 Post；放在移动计算之后、C03 之前虽然包序正确，却会让未减速的移动对应服务器的格挡状态，导致 NoSlow。当前不采用这两种时机。

正常重新格挡已与上次攻击隔开移动包，无需伪造实体交互。若其他模块在当前区间发过攻击，LagNewIO 仍要求有效交互；射线失效时等待下一移动区间，不直接发送裸 C08（PacketOrderJ）。输入攻击标记由原生出站观察更新，队列回放不重复更新。

## AntiKB 协作

KillAura 通过 AttackCycleActive 交出攻击调度，但继续提供目标和转头。Watchdog 缓存击退或恢复期间可以禁止出站 Blink，不能同时禁止 Lag New 攻击，否则双方互相等待释放。

无法启动缓存且没有其他出站队列占用时，Lag New 以相同阶段直接运行，由现有 Watchdog 发包适配层协调确认包。AntiKB 已释放我们的队列时保留阶段和移动边界，不重启攻击。Reduce/Velocity 攻击优先级和其他模式的队列所有权保持不变。

## 验证

Client/test/combat/autoblock/run_autoblock.ps1 验证三 tick 攻击循环、60 tick 中 20 tick 格挡减速、超时、手动点击、跳过移动和清理；简化的 Post/I/J 判定覆盖旧包序，额外比较每个移动包对应的本地物理使用状态与服务器包流使用状态，明确检出旧的晚格挡缺陷。此模型不替代完整 Grim 移动预测器。

Client/test/combat/antikb/run_watchdog_slinky.ps1 编译实际 LagNewIO、状态机、AntiKB 和原生队列，检查攻击释放、连续击退、确认包顺序、世界恢复、其他队列隔离、射线丢失和外部攻击插入。已有 AutoBlock 模式回归一并运行。

实机注入与服务器测试由用户完成。离线结果不证明服务器验收，也不证明缺失 Expo 依赖的一致性。

各档位另以 420 tick 检查攻击总数、格挡减速 tick 数、重复回调、Post/I/J 与客户端/服务器使用状态一致性。配置测试覆盖所有档位保存/加载和默认 7APS，手动模式测试确保切档和超时不生成点击。
