# Starain Rotation / MoveFix 逐行核对

参考源码为本项目的 `skid/Starain-1.20.1-main/src/main/java/today/starain/utils/`。
移植源码位于 `src/dev/myth/utils/`，保留 Starain 的类名、子包、方法、字段、注解、常量、分支、调用顺序和随机计算。

`scripts/verify-starain-rotation.ps1` 将参考源码仅应用下表列出的改动后，与目标文件全文逐行比对。
除了 CRLF/LF 行尾编码，未列出的语句、注释、注解或空行变化都会使校验失败。

| 工具类 | 原始行数 | 核对结果 |
| --- | ---: | --- |
| `rotation/Rotation.java` | 20 | 仅替换包名、import 前缀 |
| `rotation/RotationManager.java` | 240 | 包名及下表列出的 26.2 接口适配 |
| `rotation/RotationUtils.java` | 237 | 包名及方向法向量接口适配 |
| `rotation/RayCastUtil.java` | 178 | 包名及 partial tick 接口适配 |
| `player/MoveUtils.java` | 160 | 包名及输入字段读取适配；`fixMovement`、`direction` 方法逐行原样保留 |
| `math/MathConst.java` | 31 | 仅替换包名 |
| `vector/Vector2f.java` | 21 | 仅替换包名 |
| 合计 | 887 | 全文比对通过 |

必要的版本差异如下。这些改动在核对脚本中逐项固定，不允许其他改写。

| 原始代码位置 | Starain 1.20.1 | Myth 26.2 | 对应行为 |
| --- | --- | --- | --- |
| 全部 package / import | `today.starain` | `dev.myth` | 迁入本项目命名空间 |
| `RotationManager.smooth` | `mc.gameRenderer.pick(1.0F)` | `((MinecraftAccess) mc).myth$pick(1.0F)` | pick 已移到 Minecraft 私有方法；Invoker 调用原版方法，立即更新鼠标射线 |
| `RotationManager.onPositionItem` | 改写 `PosRot` 的 yaw / pitch | 改写 `ServerboundUseItemPacket` 的 yaw / pitch | 26.2 在使用物品包内携带转向，保留手和预测序号，保留原来的 pitch clamp 和最高优先级 |
| `RotationUtils.calculate` 两个带 Direction 的重载 | `getNormal().getX/Y/Z()` | `getStepX/Y/Z()` | 26.2 对同一 normal 向量的分量读取接口；计算式未改 |
| `RayCastUtil.rayCast` | `mc.getFrameTime()` | `mc.getDeltaTracker().getGameTimeDeltaPartialTick(false)` | 获取渲染插值时间；射线距离、候选过滤、包围盒扩张和遮挡判断未改 |
| `MoveUtils` 的输入读取 | `input.forwardImpulse / leftImpulse` | `MovementInputAccess.forwardImpulse / leftImpulse` | 读取修正后的原始输入值，按原逻辑应用潜行系数；不会将斜向归一化后的 0.707 当作原始按键值 |

配套接入在工具类之外：

- `Myth.onInitializeClient` 按 Starain 的顺序，在功能模块初始化前注册 `RotationManager.newInstance()`。
- `MixinKeyboardInput` 按 Starain 从方向键计算 -1 / 0 / 1 的 forward / strafe，再派发 MoveInputEvent；修正后交给 26.2 的向量输入流程，保留原版长度限制和潜行系数。
- `MovementInputAccess` 提供旧输入字段的等价读取，接口放在 `dev.myth.access`，可直接访问。
- `MinecraftAccess` 与 `MixinMinecraft` 为迁移后的 private pick 提供 Invoker。
- `Wrapper` 仅提供这些工具所需的原始 `Minecraft mc = Minecraft.getInstance()` 字段；未引入 Starain 的独立配置目录。
- Lombok 使用参考项目相同的 `1.18.46`，配置为 `compileOnly` 和 `annotationProcessor`，保留原始的 Data / Getter / Setter / AllArgsConstructor 行；不会将 Lombok 加入运行时依赖。

保留原始细节：MoveFix 的 45 度分段和 22.5 度容差、原始的正负号与边界包含关系、按原输入最大绝对值恢复幅度；平滑的随机偏移、随机调用次数、GCD 修正、每 tick 的 smoothed 标志；各事件的 priority 和 active / null 条件均不改写。
例如 AttackYaw / FallFlying 原本只检查 rotations 非空，未额外增加 active 条件；RayCastUtil 的无 Rotation overBlock 重载原本固定使用 UP / false，仍保持原样。

验证命令：

```powershell
.\scripts\verify-starain-rotation.ps1
.\gradlew.bat build --console=plain
```

独立客户端验证代码和结果放在 `build/rotation-validation/`，使用单独的游戏目录和临时平坦世界，不包含在正式 JAR 中。

运行结果：44 项检查通过。其中 MoveFix 将 Starain 原始 MoveUtils 作为独立参考实现，比对 4,350 组输入、yaw 和边界组合，结果全部相同。
另外验证了八个真实键盘方向与静止状态的世界移动方向及速度、原始输入适配、射线的方块/实体命中和遮挡、pick 立即刷新、PRE 运动包、使用物品包、模型渲染插值、冲刺跳跃，以及禁用/重生后的状态。
结果文件为 `build/rotation-validation/rotation-result.txt`。
