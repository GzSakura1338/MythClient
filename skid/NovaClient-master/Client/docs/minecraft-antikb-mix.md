# AntiKB Mix

参考文件：`C:/Users/13366/Downloads/AntiKnockBack.java` 的 `Mix` 分支。
实现：`src/minecraft/modules/combat/antikb/mix/`，仅使用原生 C++/JNI。

入口为 Combat → AntiKB → Mode → Mix，参数收在单独的 Mix 齿轮中。
原来的模式索引 0–3、默认 Delay 和已有配置键均保留，Mix 追加为索引 4。
Mix 参数使用独立分组，避免覆盖 Prediction 的同名参数。

## 参数及主流程

| 参数 | 默认 | 范围/用途 |
|---|---|---|
| Delay Ticks | 2 | 1–5 tick；关闭 Delay Till On Ground 后才参与超时释放 |
| Reduce Ticks | 3 | 1–6 tick 的攻击尝试窗口，失败的 tick 也消耗次数 |
| No Bad Packets | 关闭 | 启用额外的发包状态检查，见下方适配说明 |
| Jump | 开启 | 释放后的一次跳跃检查 |
| Air Push | 关闭 | 准星没有生物时寻找距离至少 11 格的最远玩家/生物 |
| Delay Till On Ground | 开启 | 等待落地释放；不是无条件在 2 tick 后释放 |
| Delay On Ground | 关闭 | 仅关闭上一项时显示；值参与原分支的延迟判断 |
| Fake Check | 开启 | 收到自身受伤状态后，正 Y 的自身击退才触发 |

接收自身 S19 状态 2 后记录受伤，正 Y 的自身 S12 按参考文件的条件进入入站延迟。
落地、液体/蛛网或配置的延迟 tick 条件满足时，按 FIFO 释放。
没有进入延迟的有效击退直接开启 Reduce Ticks 窗口。

每次尝试先消耗一个计数。准星指向生物、正在疾跑、不在格挡且攻击门控允许时，
发送 C0A → C02 ATTACK，然后将本地 X/Z 速度各乘 0.6，保持 Y，关闭本地疾跑。
不调用 vanilla attackEntity，不主动恢复疾跑，不凭空重复消耗同一个击退。
参考文件这里没有 Team/AntiBot 过滤，移植也未额外添加。

释放后的 Jump 在落地、按住前进、没有跳跃药水且不在液体/蛛网时请求一次跳跃，
不要求疾跑；不满足条件也消耗此次跳跃检查。关闭 Jump 时保留标记，遵循原分支。

Air Push 保留源文件“至少 11 格且选最远”的判断，不改成最近目标。
发送远程攻击包不代表服务器承认伤害或击退减免。

## 缺失依赖及 Nova 适配

提供的文件不包含 DelayComponent、BadPacketsComponent、PlayerUtil 实现，
也没有 `canReduce` 的外部赋值代码。因此不能声称这些组件完全复刻。

- DelayComponent：使用 Mix 独立 JNI 全局引用队列；延迟期间缓存后续入站包，keepalive 直通。
  事务包到达时先释放旧队列再处理事务；S08 先释放旧队列后重置攻击/跳跃状态。
  切世界、重生、断连、死亡和玩家实例替换丢弃旧世界缓存。
  关闭模块/切模式在同世界释放，在世界已替换时丢弃。8192 包上限或分配失败时释放并放行。
  不添加固定毫秒超时来替代参考文件的落地条件。
- `canDelay`：落地，且 KillAura 没开启，或当前没有 AutoBlock/格挡目标。
  Nova 没有对应 Fake 模式。
- `canReduce`：KillAura 使用 Nova 已有的 VelocityCanReduce 门控。
  Lag New 拥有独立攻击/格挡周期，启用它时 Mix 的额外主动攻击让路，延迟与跳跃仍工作，
  不向 AutoBlock 提交会相互等待的攻击队列，也不清空其他模式的出站缓存。
- No Bad Packets：使用 Nova 的交互、使用、挖掘、关容器、切槽和本周期攻击状态检查；
  不是缺失的原组件的逐包等价实现，仍保留原默认关闭。
- PlayerUtil.isMob：使用 Nova 的 Monster/Animal/Villager 类型；距离使用眼睛到实体包围盒距离。
- 原 Update PRE 的主动攻击接到 PrePlayerInteractEvent，在使用物品物理计算和 C03 前执行；
  释放在 PostPlayerTickEvent；跳跃在 LivingUpdateEvent。
- 当前运行时使用 legacy Lunar 适配层，其他版本只保留模式/参数，不执行此 1.8 包逻辑。

## 验证

`test/combat/antikb/run_mix.ps1` 编译真实 Mix 状态机、运行时和设置代码，以模拟 JNI/游戏环境验证：
受伤和 Y 条件、落地/按 tick/液体释放、攻击和单次 0.6 摩擦、疾跑和 AB 门控、跳跃、Air Push、
FIFO 与引用释放、世界/玩家更换、S08/事务、禁用和模式切换。

另运行 AntiKB Delay/Reduce、Watchdog/Slinky/Lag New 协作和 AutoBlock 回归脚本，
以及 Release DLL 构建与静态 TLS 检查。测试不等同于实服反作弊兼容验证；由用户自行注入测试。
