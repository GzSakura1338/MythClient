# Breaker

来源：`skid/Rise-6.9.5-main/src/main/java/com/alan/clients/module/impl/player/Breaker.java`，以及它依赖的 `BreakCandidate`、`RotationComponent`、`BlinkComponent` 和 KillAura 拆床判断。

入口为 **Player → General → Breaker**，默认关闭。参数位于模块的小齿轮中，使用现有配置保存、快捷键和 ArrayList 注册机制。项目内没有新增 Java 源码。

## 设置

| 设置 | 默认值 | 行为 |
| --- | --- | --- |
| Mode | Normal | Normal 调用游戏挖掘控制器；Instant 发送 START/STOP、两次挥手，执行本地破坏并冷却 20 tick |
| Range | 4.5 | 1–6，步长 0.1 |
| Bed | 开 | 选取床及其需要移除的保护方块 |
| Keep Break Progress When Out Of Range | 开 | 保留控制器进度字段；实际服务端破坏状态仍由服务端决定 |
| Cancel Velocity Whilst Breaking | 开 | 有破坏目标时取消本地玩家击退包 |
| Delay Velocity Until Bed Broken | 关 | 暂存本地玩家击退和其后的确认包，结束拆床后释放 |
| Through Walls | 开 | 允许选取射线后方的床 |
| Empty Surrounding | 关 | 先处理保护方块；仅 Through Walls 开启时显示 |
| Surroundings | Single | Single 处理相邻保护；Full 比较穿过多层保护的候选路径 |
| Surrounding Layers | 3 | Full 的保护层上限，1–6 |
| Show Best Stand Position | 关 | Full 中每 10 tick 搜索一次有支撑、可站立且破坏成本最低的位置 |
| Heypixel | 关 | Single 相邻保护搜索只取上方 |
| Watchdog Ground Spoof | 关 | START/STOP 激活后，在运动事件修改发送的落地状态并暂存相关网络流量，实际落地时释放 |
| Attack While Breaking | 开 | 控制 KillAura 在拆床期间是否允许攻击，也接入现有 Hypixel AutoBlock 攻击入口 |
| Show Breaker Percentage | 开 | 显示准星下方的进度 HUD |
| Rotate | 开 | 通过 Nova 的统一旋转管理器提交转头 |
| Only Rotate at Start and Stop | 开 | 控制器进度为零或即将完成时提交转头 |
| Whitelist Own Bed | 开 | 记录超过 40 格的服务端位置修正终点，35 格内不选床 |
| Slow Down In Air | 开 | 关闭时只在挖掘调用期间临时设置本地 onGround，并在调用结束或 JNI 异常后恢复 |
| Movement Correction | Off | Off / Rise / Traditional / Backwards Sprint |

## 搜索与交互

保留原实现的 ±5 格扫描顺序、普通搜索跳过首个床半块、Full 的五个采样点、最多 64 个体素的 DDA、按相对破坏硬度计算成本、移动 0.5 格且间隔 5 tick 才重新比较路径，以及 10% 换路收益阈值。

Normal 使用对应版本的原版控制器推进破坏。专用调用重定向防止原版 `continueAttack(false)` / `sendClickBlockToController(false)` 同一 tick 清空模块正在推进的进度，不模拟物理鼠标持续按下。每次操作提供目标 BlockHitResult。`BlockDamageEvent` 将目标交给现有 AutoTools，控制器负责实际工具、硬度、挖掘包和本地破坏规则。

JNI 调用使用有界局部引用帧；方块读取按 tick 缓存，跨线程只交换值快照或全局包引用。网络回放始终在队列锁外进行。模块关闭释放待处理包；断线/换世界丢弃旧连接的待处理包。

## 原生适配与有意差异

- Single 源码中的 `akax2.equals(akax2)` 恒为真，并在循环中修改搜索基准。这会使完整包床时无法正确选择保护块。移植固定基准并返回硬度最低的候选方块，未复制这个错误。
- 原源码的 `Bed` 属性未参与选床判断；Nova 让它实际控制选床。
- 不将 `Slow Down In Air` 临时落地状态泄漏到后续游戏逻辑；只包围控制器调用。
- 旋转复用 Nova 的现有平滑与敏感度处理，不另建 Rise 全局旋转组件。动作时序通过 Nova 事件执行，不能将其宣称为 Java 字节码逐条相同。
- Blink 使用独立所有权，保留确认流量与出站交互包顺序。旧版聊天和补全包、高版本聊天/命令包不暂存；不会接管其他模块已经取消的包。队列增加 4096 包和 5 秒上限，避免无限积压。
- 打开游戏界面或 ClickGUI 时暂停操作；关闭后重新选择目标。
- UI 使用 Nova HUD 背景、字体和世界投影，目标框以 50ms 线性高度动画表现进度。没有复制 Rise 的 OpenGL Bloom 管线。Nova 没有 FastBreak 模块，因此 Rise 对 FastBreak 额外进度加成的分支按未开启处理。
- 1.20.1 与 26.3 使用各自游戏控制器和网络协议。1.8.9 的即时挖掘/落地包技巧不能据此保证在高版本或任何服务器有效。

## 文件和验证

- [模块、网络和渲染](../src/minecraft/modules/player/breaker/Breaker.cpp)
- [选床、保护路径和站位算法](../src/minecraft/modules/player/breaker/BreakerLogic.h)
- [JNI 控制器与方块适配](../src/minecraft/java/game/GameBreaker.cpp)
- 所有版本成员均在 `src/minecraft/compat/mappings/Lunar189.inc`、`forge1201.inc`、`minecraft263.inc` 的 `NOVA_BREAKER_MAPPING` 节中。
- [离线测试](../test/player/breaker/run.ps1)：三个实际版本 class 的成员核对、实际 Minecraft 方法重写及栈映射校验、一万组独立射线相交核对、裸床/包床/层数/距离/不可破坏方块/己方床/站位测试。

构建输出：`Client/build-vs2026/Release/NovaClientBase.dll`。构建会执行 PE 静态 TLS 检查。没有自动注入或进行在线行为验证。

用户实测建议：先在单人世界用普通工具测试裸床 Normal，再测试羊毛包床 Single 和 Full；检查关闭、离开范围、打开界面以及换世界后能否恢复手动挖掘。Instant 和包延迟选项分别测试，最后再测试与 KillAura、AutoTools、AutoBlock 的组合。
