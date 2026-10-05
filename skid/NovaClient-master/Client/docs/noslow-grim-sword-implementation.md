# NoSlow / GrimSword 完整实现规格

日期：2026-09-23。目标仓库：`D:/AI/Minecraft/Nova`。

本文面向实际编码与验收，补充 [运行逻辑分析](noslow-grim-sword-design.md)。新增文件名、接口名和状态均为拟实现规格；标明“现有”的接口才表示仓库已经具备。当前交付仅为文档，未改模块、游戏归档或 DLL。

## 1. 交付目标与完成条件

目标客户端为本机 Lunar 1.8.9。NoSlow 只作用于正在使用的剑格挡，不改变食物、药水、弓，也不因“手持剑但未使用”而修改运动。

必须分别交付并记录三项能力：

| 能力 | 定义 | 当前证据 |
| --- | --- | --- |
| 本地步行无减速 | 相同输入、地形和初始运动状态下，取消剑使用造成的输入 0.2 倍率 | 已定位原因和可用原生执行位置；待实现测试 |
| 格挡期间疾跑 | 恢复相应疾跑启动/维持行为，保持其余合法疾跑条件 | 需要额外调用点分析与实现，不能由步行结果推定 |
| GrimSword | 目标环境下全速移动、实际格挡防御和 Grim 检测同时达到验收要求 | 尚无经验证协议策略，属于待闭合依赖 |

“100%”以相同条件下未使用物品的对照为准，不以每 tick 固定位移、动画、客户端速度数值或短时间未回弹作为证明。步行与疾跑分别报告。连续格挡防御与间歇格挡分别报告。

本文件提供完整客户端实施路径、Grim 策略接入契约及验证流程；未验证的协议细节不能编造为可直接上线的 Grim 绕过算法。GrimSword 未闭合时不得把本地模式当作它交付。

## 2. 依据与适配边界

Recaf 对象为 Lunar 启动日志对应的完整类归档：

`C:/Users/13366/.lunarclient/offline/multiver/cache/72c13d03/4b04e698/bake.zip`

共 19,946 个类，包含 Lunar 注入。不是未经修改的 Mojang JAR。已经检查 ItemSword、EntityPlayer、EntityPlayerSP、Minecraft、PlayerControllerMP，以及归档中的原版服务端使用/释放处理。

关键顺序：

1. 右键路径调用控制器：同步槽位 → C08 使用包 → 物品右键 → setItemInUse。
2. 玩家更新：检查使用物品身份 → 更新移动输入 → 物品使用输入乘 0.2 → 疾跑判断 → 父类运动 → 行走包。
3. 松键：控制器同步槽位 → C07 RELEASE_USE_ITEM → 本地 stopUsingItem。

Grim 分析对象为 `skid/Grim-2.0`，baseVersion 为 2.3.74。源码可以解释判定逻辑，不能证明远端构建、配置及 Via 链路相同。

实现仅为 Lunar 1.8.9 打开能力位。Lunar 1.7、1.21.8、Forge、EC 等不得因共享 Runtime 或同名模块而自动启用。

## 3. 文件与职责

以下路径均相对仓库根目录。

| 文件 | 操作 | 职责 |
| --- | --- | --- |
| `Client/src/minecraft/modules/movement/noslow/NoSlow.h` | 新增 | 不含 JNI 的门控、模式、原因枚举与状态模型 |
| `Client/src/minecraft/modules/movement/noslow/NoSlow.cpp` | 新增 | Module、属性、启停、状态显示与策略选择 |
| `Client/src/minecraft/modules/movement/noslow/NoSlowRuntime.h/.cpp` | 新增 | 原生执行接口、输入快照、生命周期、诊断计数 |
| `Client/src/minecraft/modules/movement/noslow/GrimSwordStrategy.h/.cpp` | 后续新增 | 通过验收的协议策略；未验证阶段只提供不可用结果，不发包 |
| `Client/src/minecraft/modules/Registry.h/.cpp` | 修改 | 声明并注册 MakeNoSlowModule |
| `Client/src/minecraft/modules/CMakeLists.txt` | 修改 | 显式登记新增实现文件；现有构建不靠目录自动扫描 |
| `Client/src/minecraft/compat/lunar189/LunarRuntime.cpp` | 修改 | 在 InputHook、LivingUpdateHook 和生命周期处接入原生接口 |
| `Client/src/minecraft/compat/mappings/Lunar189.inc` | 按需修改 | 仅补缺失字段/方法或后续疾跑调用点；不替换其他版本映射 |
| `Client/test/movement/noslow/` | 新增 | 原生状态/生命周期与输入差分测试、只读归档结构检查 |
| `Client/test/run-offline.ps1` | 修改 | 登记新测试入口 |

不新增 Java 源码、Java 模拟器或嵌入式 Java helper 字节码。模块状态逻辑采用 C++，游戏对象读取采用现有 JNI 基础设施。

现有 `modules/combat/keepsprint/KeepSprint.cpp` 只处理攻击减速事件；不要把它误当成使用物品减速模块。

## 4. 模块与设置

模块名 `NoSlow`，分类 `Category::Movement`，默认关闭，沿用 Module 的配置与绑定机制。

建议属性：

| 属性 | 值与默认 | 行为 |
| --- | --- | --- |
| Mode | Local / GrimSword；初期仅展示已可用模式 | Local 完成本地输入修正；GrimSword 必须满足策略能力条件 |
| Sprint | false | 初期能力未实现时不展示；不得让开关成为无效承诺 |
| Debug Trace | false，仅开发配置 | 有界原生诊断记录，不每 tick 输出文本 |

SwordOnly 是范围约束而非可关闭选项，不提供 Food、Bow、Consume 配置。不要新增未经需求支持的全局倍速、Timer、运动向量倍率。

启用时读取能力：runtime 类型、必要映射、输入钩子和运动前钩子都满足才可进入 Armed。GrimSword 还要求策略验证信息匹配；不可用时保持禁用并显示简短原因。配置导入同样受此约束。

## 5. 采用的本地实现路径

### 5.1 已存在的钩子

`Lunar189.inc` 已把 `onLivingUpdate` 内的 `MovementInput.updatePlayerMoveState()` 重定向到 InputHook，并把调用父类 `AbstractClientPlayer.onLivingUpdate()` 的位置重定向到 LivingUpdateHook。

现有 InputHook：调用原输入更新 → 派发 MoveInputEvent → 应用潜行倍率及 finalForward/finalStrafe → 写回输入字段。

现有 LivingUpdateHook：派发 LivingUpdateEvent → CallNonvirtualVoidMethod 执行父类运动。

这提供了“减速之前捕获，运动之前恢复”的两个原生执行位置。无需首先设计新的字节码插入器。早期分析中尚未确认专用钩子的部分，由此次仓库核对补齐。

### 5.2 输入快照

在 InputHook 的所有输入事件、潜行和最终值处理完成，JNI 字段写入成功之后捕获：

- 当前世界 generation、玩家 tick generation、输入调用 serial；不使用系统时间充当 tick 身份。
- 模块配置 revision、运行模式、输入 forward/strafe。
- 玩家、MovementInput、当前物品和使用物品的身份验证令牌。
- 捕获时的 using、sword、BLOCK、riding、alive 状态。
- 是否有其他模块占用输入/使用物品控制权。

输入归一化、旋转修正、潜行和其他模块产生的最终输入应全部保留。快照不是键盘原始值，也不强制改成 -1/0/1。

如果没有使用物品、物品不是剑、骑乘、快照数据非有限值、对象读取失败、能力不可用，清空当前快照并退出。

InputHook 当前含“没有 MoveInputEvent 监听器则返回”的优化。接入后必须让 NoSlow 的捕获需求参与该判断：有捕获需求但没有监听器时也读取最终输入；不能依靠一个无意义的空订阅维持工作。

### 5.3 运动前修正

在 LivingUpdateHook 原有 LivingUpdateEvent 派发完成后、调用父类运动之前执行修正。理由：保留现有事件顺序，并可以检查监听器是否改动了输入。

按顺序执行：

1. 本地玩家、同世界、同 tick、同配置 revision、同 input serial，且快照尚未消费。
2. 用 JNI 身份比较确认玩家/输入对象/物品未变化。不得用 jobject 的数值比较代替 IsSameObject。
3. 再次读取使用状态、剑类型、BLOCK、骑乘和生命周期；任何变化均放弃。
4. 检查输入仍对应“快照 × 原版 0.2”。两轴分别检查；若其他模块或 Lunar 新逻辑已改写，不用旧快照覆盖它们。初始实现选择整次放弃，不做一轴成功、一轴失败的混合修正。
5. Local 模式满足门控后写回快照的 forward/strafe；GrimSword 必须另有本次预测阶段的策略许可。
6. 标记快照已消费，再执行原来的父类运动调用。重复进入不得重复修正。

原版减速确认使用 float 运算预期值，允许极小 ULP/绝对误差以应对工具与记录转换；建议起始绝对容差 1e-6，并通过差分测试收紧。这个值是本地一致性校验参数，与 Grim 的 0.001 预测误差阈值无关。

不要把当前 motion 乘 5，也不要用 input 除 0.2 去反推可能已被其他模块改写的值。保持 jump/sneak、世界速度、摩擦、碰撞、击退和 Timer 原状。

### 5.4 重要限制：疾跑已先执行

恢复位置位于原版疾跑判断之后，因此本路径交付的是步行基线消除减速；既有疾跑可能已因 0.2 输入而结束，不能通过晚写输入声称已经保留疾跑。

若追加 Sprint 能力，需独立分析该归档全部相关调用点，在疾跑判断之前恢复输入并只针对剑调整使用物品限制，同时保留饥饿、碰撞、失明、骑乘等原条件。优先使用现有原生调用点重定向能力；必须核对调用点上下文和数量，不能全局替换 isUsingItem 的返回值。

该增强未验证前不开放 Sprint。不要在运动前简单强制 setSprinting(true)，否则会跳过原条件并与 Sprint/KeepSprint 模块冲突。

## 6. 原生接口契约

以下为拟新增接口的语义，不表示仓库已有对应函数：

| 接口 | 输入 | 输出/副作用 |
| --- | --- | --- |
| QueryCapabilities | runtime 与映射校验结果 | inputCapture、prePhysicsRestore、sprintControl、protocolStrategy 能力位 |
| BeginPlayerTick | world/player generation | 递增本地 serial，清除上一 tick 未消费快照 |
| CaptureInput | JNI 临时上下文、最终输入、generation | 只保存一次本次调用快照；不发包 |
| BeforeParentLivingUpdate | 当前上下文与策略许可 | 原子地决定两轴是否恢复，返回 Applied/Skipped 与原因 |
| ObserveOutbound | 已通过取消/队列处理的实际出站包摘要 | 更新观测状态，不推定服务端确认 |
| Reset | 原因、是否仍连接 | 释放引用、丢弃快照、撤销控制权；默认不发包 |
| ReadDiagnostics | 只读 | 计数与近期环形记录 |

纯逻辑层接收已解码的值，避免依赖 JNIEnv。JNI 层负责读取、对象身份和异常传播。UI 读取统计快照，不持有游戏 jobject。

## 7. 状态机

| 状态 | 含义 | 主要转移 |
| --- | --- | --- |
| Disabled | 模块关闭 | 启用且能力满足 → Armed |
| Unavailable | 版本/映射/策略不可用 | 能力重新验证成功后允许重新启用 |
| Armed | 已启用，未满足剑使用条件 | 有效使用剑 → Captured |
| Captured | 当前 tick 已有输入快照 | 运动前门控成功 → Applied；失败 → Armed |
| Applied | 当前快照已消费 | 新 tick → Armed；关闭/重置 → Disabled |
| Suspended | 其他模块占用、需要让出控制 | 占用解除且下一 tick 重验 → Armed |

每个状态带 worldGeneration/configRevision，旧世代事件直接丢弃。状态重置不自动“补一次右键”或恢复旧剑。

Grim 观测状态单独保存 Unknown、UseSent、ReleaseSent、SlotChanged；这些名称只描述客户端已观察到的出站行为。禁止使用 ServerBlockingConfirmed 之类未经服务端数据支撑的名称。

## 8. JNI、线程与异常

所有游戏对象操作在对应游戏回调线程执行。网络线程仅传递解码后的值记录；跨线程采用现有调度机制，不能共享 JNIEnv。

输入快照跨两个 JNI 回调存活，不能保存普通 local reference。优先使用游戏 tick 内可重取的对象及稳定 generation；需要跨回调严格身份校验时，使用有界 global reference，并在消费、丢弃、reset、卸载时 DeleteGlobalRef。线程退出不等于这些引用已释放。

GetField/CallMethod 每阶段检查异常。因诊断/门控读取新增的异常按现有 Runtime 规则处理并停用该次修正；原游戏逻辑的异常不应被模块吞掉。若两轴写入中途失败，记录不完整写入并进入 Runtime 现有故障处理流程，避免带 pending exception 继续进行无关 JNI 调用。

不要持锁调用 EventBus、Java 方法或发包。销毁模块前撤销订阅，停止回调访问已释放状态。NoSlow 不改变 DLL 原有卸载顺序。

## 9. GrimSword 协议策略契约

### 9.1 已确认的约束

- 有效剑使用会令本地 Grim 的 slowedByUsingItem 为 true。
- 使用状态下输入预测乘 0.2；翻转使用状态候选不会自动取消 NoSlow 检查。
- C07 RELEASE_USE_ITEM 清除 Grim 使用状态，同时原版服务端停止使用物品。
- 切槽处理依赖使用槽、当前槽、移动采样及 tick 可靠性等条件；同槽 C09 不能直接当作免检操作。
- 1.8 的 didSlotChangeLastTick 分支没有跳过后续 NoSlow 误差处理。

### 9.2 策略必须提供的结果

策略每次提交决策时返回：是否允许本 tick 本地输入修正、所依赖的阶段/序号、控制权、已排队动作及其取消规则、有效环境签名、预期是否中断实际防御。

初始未验证策略固定返回 Unsupported，不发送探测包，也不偷偷退回 Local。只有对应版本与配置的测试证据齐备后，才能提供有效 AllowLocalRestore。

策略不通过直接调用 sendUseItem 再根据其布尔返回值判断成功；这个返回值不等价于格挡成功。也不把写入队列视为实际发送完成。

### 9.3 需要闭合的验证工作

先记录正常右键/松键/切槽的客户端发送与服务器接收顺序，对齐 Grim 每次受检预测和实际服务端使用状态。随后验证任何候选时序是否同时满足运动许可和防御指标。使用本地 Grim 的断点、已有诊断工具或可用服务器观测设施；不为此在 Nova 编写 Java 插件。

本地日志只能证明客户端状态与发送顺序。服务器使用状态不可观测时，把防御结论标为未确认；缺少 Grim 检测输出时，不以“没有回弹”替代。

在这些数据缺失时，本规格不提供虚构的“每 tick 固定发几个包即可稳定绕过”步骤。这是 GrimSword 成品的明确剩余依赖，而非本地钩子已经解决的问题。

## 10. 模块协作与生命周期

| 场景 | 规则 |
| --- | --- |
| AutoBlock 正在控制使用/释放或延迟包 | 首版让出控制并 Suspended；只有明确所有权协议接入后才允许组合 |
| Scaffold/其他模块切槽或 spoof 槽位 | 立即废弃快照；下一 tick 重新读取真实物品身份 |
| AntiKB Prediction 改写输入 | 不用旧快照覆盖；不满足 scaled-input 校验即跳过 |
| KeepSprint 处理攻击减速 | 保持它的事件；NoSlow 不更改攻击减速系数 |
| GUI/物理右键与自动使用不一致 | 以实际 itemInUse + 所有权判断，不能只看按键 |
| 剑换另一把剑 | 即使类型相同也视为物品身份变化 |
| 死亡/换世界/断线 | generation 失效、清引用、丢待处理动作，不发送历史释放/恢复包 |
| 普通关闭 | 停止本地输入修正；Local 从未接管使用状态，不额外停止玩家真实右键 |

现有 AutoBlockPacketSync 的 SynchronizePacketDelays 只协调 Slinky/Hypixel 输出。它不是通用物品使用锁；不能无条件调用它清队列以“解决冲突”。Grim 策略若拥有待发送动作，必须通过对应队列的所有权机制撤销自己的动作，不能丢弃其他模块的包。

## 11. 诊断记录

采用有界环形缓存，例如 512 条，Debug Trace 关闭时只保留必要计数。字段包括：world/tick/input serial、模块模式、状态、跳过原因、两轴捕获/当前/预期/写回值、using/sword/riding/sprint、物品身份变化标记、实际出站包类型和顺序号。

原因枚举至少包括 UnsupportedRuntime、HookUnavailable、NotUsingSword、Riding、ItemChanged、StaleSnapshot、AlreadyConsumed、InputConflict、OtherOwner、ProtocolUnavailable、JniFailure、Disconnected。

诊断不记录认证信息、聊天或完整网络负载。一次性导出由开发命令触发，禁止游戏线程每 tick 磁盘写入。

## 12. 实施顺序与阶段出口

1. **能力与结构校验**：确认本机归档中两个现有 redirect 都匹配，读取生命周期和引用约定，记录目标方法/归档标识。出口：只读校验能发现缺失或重复匹配。
2. **纯 C++ 状态模型**：实现模式门控、generation、输入快照消费、跳过原因。出口：原生状态测试通过。
3. **Local 接入**：修改 InputHook 提前返回条件，捕获最终输入，在 LivingUpdateHook 运动前恢复，注册模块及显式构建源。出口：开启/关闭和非剑路径符合差分预期。
4. **生命周期与共存**：验证引用释放、异常、卸载和各模块冲突。出口：无跨 tick 恢复、无遗留引用、无隐式包改写。
5. **Grim 观测**：固定服务器环境，采集正常与 Local 对照。出口：能够把运动样本、检测误差和实际防御对应起来。
6. **Grim 策略闭合**：依据观测实施并验证候选策略。出口：所有必需指标通过；失败保持不可用，记录具体失配。
7. **可选 Sprint**：独立实现与验收。出口：正常疾跑条件未被绕开，且相应 Grim 环境结果有证据。

步骤 3 完成只可以报告 Local 实现完成；不能报告 GrimSword 完成。

## 13. 测试与验收

### 原生自动测试

测试行为和不变量，不仅复刻实现：

- 非剑、未使用、骑乘、关闭时输出与输入相同，不产生包动作。
- 原版减速输入与快照匹配时恰好恢复一次；潜行/斜向最终输入保留。
- generation/config revision/物品身份改变后旧快照不可应用。
- 重复运动回调、遗漏输入回调、连续两次输入回调不会消费错误样本。
- 其他输入修改导致跳过，不覆盖新值；NaN/Inf 不进入写回。
- 切换模式与关闭不创建历史发送动作；不可用 Grim 策略无法启用输入恢复。
- 引用管理测试替身能检测重复释放、未释放和 local ref 跨回调误用。

### 游戏差分验证

分别记录模块关闭、原版格挡、Local 三组；同世界、地面、初始速度、视角和按键。每组先完成预热，再记录至少 200 个有效运动 tick，多次重复。该样本量是初始测试基线，不是稳定绕过证明。

比较逐 tick 输入与位移轨迹；平地稳定步行速度比应接近未格挡对照，建议初始验收误差不超过 1%，出现差异追踪输入与物理状态，不用加速补偿掩盖。跳跃、斜向、潜行、碰撞与击退各自使用对应对照，不能套用平地固定速度。

### 场景矩阵

| 组 | 场景 | 通过条件 |
| --- | --- | --- |
| 范围 | 五种原版剑；食物/药水/弓/空手 | 剑使用时可作用，排除项与原版一致 |
| 操作 | 按住、松开、快速重复、换剑、换食物 | 无卡格挡、旧输入恢复或旧物品复活 |
| 运动 | 直线、斜向、潜行、跳跃、碰撞、受击 | 对应输入与物理约束保持 |
| 生命周期 | 开关模块、死亡、重生、切世界、重连、卸载 | 无陈旧引用、残留策略动作或崩溃 |
| 共存 | AutoBlock、Scaffold、AntiKB Prediction、Sprint、KeepSprint | 按所有权规则退出或经验证协作 |
| 网络 | 正常延迟、抖动、tick 跳过 | 记录实际顺序与预测，不用平均延迟推定 |
| 防御 | 可格挡攻击与释放/切槽附近攻击 | 服务端防御效果符合声明；动画不作为证据 |
| 检测 | NoSlow 与相关动作/包顺序检查 | 保存检测输出与配置，检查是否仅被禁用或未启用 |

完整离线测试入口仍为 `Client/test/run-offline.ps1`；新套件应有独立入口供开发迭代。具体编译命令沿用新套件及当前仓库的构建配置，不在文档假定不存在的 preset。

## 14. 实施后必须填写的结果记录

记录提交标识、Lunar 归档路径与哈希、运行时版本、钩子匹配结果、Grim 构建标识/配置、服务端版本、Via 链路、模式与共存模块、测试步骤、样本日志和失败原因。

分开填写 Local 步行、Sprint、Grim 检测、实际格挡防御四项结果；未测试标记“未测试”，不可用标记具体原因。只有四项中产品宣称支持的项目都有相应证据，才能形成最终实现说明。
