# Mio AutoCrystal 移植

参考 `skid/openmio/src/main/java/me/mioclient/module/combat/AutoCrystal.java`（1101 行）
及伤害模型 `me/mioclient/ArmorSearchHelper4.java`。
实现位于 `src/dev/myth/model/combat/AutoCrystal.java`，伤害计算在
`src/dev/myth/utils/CrystalDamage.java`，列在 Combat，默认关闭、未绑定。

## 修掉的那个 bug：不会主动放置

Mio 这份反编译源码里 `ArmorSearchHelper4.get1902`（爆炸暴露率）结尾是

```java
int i = 0, i2 = 0;   // 命中样本 / 总样本
...
return i / i2;       // 两个 int 相除
```

整数除法把暴露率截断成 0，于是 `get1900` 里
`exposure = (1 - distance) * 暴露率` 恒为 0，伤害恒为 `1 * 难度系数 - 护甲`，
基本都小于 MinPlaceDamage，所以扫描永远选不出落点，**表现就是开了不放水晶**。
原版 `ServerExplosion.getSeenPercent` 的最后一行是 `(float) hits / count`。

本移植直接调用原版 `ServerExplosion.getSeenPercent(Vec3, Entity)`：它是 `public static`，
只用到 `entity.level().clip(...)`，客户端可用，算法与 Mio 一致且除法本来就是对的。

## 伤害模型（CrystalDamage）

逐行对应 Mio 的 `get1900` / `get1903` / `get1904`：

```text
中心点 = 方块上方格子的中心（水晶实际生成位置）
脚部点 = (lerp(0.5, minX, maxX), minY, lerp(0.5, minZ, maxZ))
直径   = 6 * 2
距离   = 脚部点到中心点 / 直径；> 1 直接返回 0
暴露   = (1 - 距离) * getSeenPercent(中心点, 实体)
原始伤害 = (暴露² + 暴露) / 2 * 7 * 直径 + 1
→ 难度缩放（和平 0 / 简单 min(d/2+1,d) / 普通 d / 困难 d*3/2）
→ CombatRules.getDamageAfterAbsorb（护甲 + 韧性）
→ 抗性提升 (25 - 5*(等级+1)) / 25
→ 保护附魔因子 (1 - min(因子,20)/25)，爆炸保护按 2 倍计
```

`AssumeBestArmor` 开启时对目标按因子 20 计算，与 Mio 一致。

## 设置

沿用 Mio 的命名（括号内为 Mio 原名）。

| 设置 | 默认 | 说明 |
| --- | --- | --- |
| Place | 开 | 右键展开以下放置项 |
| PlaceDelay | 0 ms | 0～1000 |
| PlaceRange / PlaceWallRange | 4.5 / 3.5 | 视线通透与隔墙分别限距 |
| MinPlaceDamage / MaxSelfPlace | 6 / 8 | 对目标的下限、对自己的上限 |
| PlaceRotate | 开 | |
| Break | 开 | 右键展开以下破坏项 |
| HitDelay | 0 ms | 0～1000 |
| BreakRange / BreakWallRange | 4.5 / 3.5 | |
| MinBreakDamage / MaxSelfHit | 6 / 8 | |
| BreakRotate | 开 | |
| TargetRange | 12 | 选最近的非自己、非旁观、存活玩家 |
| AntiSuicide | 开 | 自伤 ≥ 当前血量+吸收时跳过 |
| FacePlace | 关 | 右键展开 FacePlaceHealth(8) / FacePlaceArmor(0%) |
| AssumeBestArmor | 关 | |
| BasePlace | 关 | 右键展开 StrictDirection(开) / Air(关) |
| AutoSwap | Normal | None / Normal；副手有水晶时直接用副手 |
| SwapBack | 开 | |
| RotSpeed | 180 | 共用现有 RotationManager |
| Render | 开 | 右键展开 Fill / Outline / LineWidth(1.5) / Fade(开) / MultiFade(开) / FadeTime(1s) |

FacePlaceArmor 沿用 Mio `get1905` 的语义：目标护甲**剩余耐久百分比**的最小值，
空槽算 0；低于阈值就忽略 MinDamage 下限。

## 流程

`MotionEvent.PRE` 里按 Mio 的顺序先破后放：

1. 选目标（最近玩家，TargetRange 内）。
2. 破坏：遍历客户端实体里的 `EndCrystal`，按距离/隔墙限距、伤害下限、自伤上限筛选，
   取对目标伤害最高的一个，转头后 `gameMode.attack` + 挥手。
3. 放置：先确认手上或副手有水晶（AutoSwap 为 None 且没拿着就直接退出），
   以眼睛高度为中心扫 ±ceil(PlaceRange) 的立方体，按原版 `EndCrystalItem.useOn`
   的条件过滤（黑曜石/基岩、上方为空、上方 1×2 内无实体），算伤害取最优，
   必要时切槽，转头后 `gameMode.useItemOn` + 挥手，`SwapBack` 再切回。

转头与放置的先后顺序和本项目 Scaffold 一致（`setRotations` 后立即 `useItemOn`），
`useItemOn` 内部会先 `ensureHasSentCarriedItem`，所以切槽包一定早于使用包。

## BasePlace（自动垫黑曜石）

对应 Mio 的 `basePlace` / `air` / `strictDirection` 与 `do1134`、
`PhaseESPSearchHelper4_2.getDirection3030` / `getList3031` / `is3042`。

扫描时除了现成的黑曜石/基岩，还会把「能垫出来」的位置一并计分：
该格可替换、在世界边界内、格子里没有实体（掉落物、经验球、经验瓶、箭矢不算），
且存在一个可点击的相邻面。伤害中心同样取 `(x+0.5, y+1, z+0.5)`，
因为水晶最终会站在垫出来的那块上。

选中的位置如果还不是黑曜石，就先走 `placeBase`：切到热栏里的黑曜石 →
按 Mio 的 `DOWN, NORTH, EAST, SOUTH, WEST, UP` 顺序找一个不可替换的相邻块 →
点它的反面（命中点 = 目标格中心 + 方向 × 0.5）→ 挥手 → `SwapBack` 切回。
下一次 place 这格就是合法底座了。

- `StrictDirection`（默认开）要求相邻块对着眼睛的那一面是可见的，
  可见面判定沿用 Mio `getList3031`：剔除被实体方块埋住的面、世界顶、
  以及背对眼睛的 ±X/±Z/上下面。
- `Air`（默认关）允许没有相邻块时直接点目标格本身的第一个可见面（Mio 的 airplace 分支）。
  Mio 那边还要 AntiCheat 放行，本项目没有该模块，所以由这个开关单独控制。
- 手上没有黑曜石时不会把「需要垫」的位置纳入候选，否则会占掉当回合本来能直接放的点。

## ESP

Mio 用 `AntiPhaseSearchHelper4` + `SpeedMineHelper_3` 在世界里画渐隐盒子。
本项目按要求**用 ImGui 画**，不接 26.2 的 gizmo/提交式管线：

- `src/dev/myth/UI/WorldEsp.java`：`Myth.WORLD_ESP` 收集每个模块提交的盒子，
  `UiManager` 在模糊前的第一遍里绘制，所以 Map 之类的模糊面板会把它一起糊进去。
- 投影取 `gameRenderer.gameRenderState().levelRenderState.cameraRenderState` 里的
  `pos` / `viewRotationMatrix` / `projectionMatrix`，即这一帧实际用的矩阵，
  FOV 变化自动跟随。8 个角投到屏幕后画 6 个填充四边形 + 12 条描边。
- 渐隐沿用 Mio：最后一次触发后 150 ms 保持满透明度，再按 `FadeTime` 秒线性淡出；
  `Fade` 关闭时只保留最新一个盒子，`MultiFade` 控制透明度是否乘进颜色。
- 设置：Render、Fill、Outline、LineWidth、Fade、MultiFade、FadeTime，与 Mio 同名。
  和 Mio 一样只在**放置**时高亮，破坏不画。

已知差异：任一角落到相机背后时整个盒子跳过不画（Mio 在 3D 里做裁剪）。
贴着水晶位站时会短暂看不到框。

## 未移植

Mio 这个模块依赖大量本项目不存在的子系统，以下按未启用处理，均**未移植**：

- **Draw（ExtraTicks 的预测轨迹线）**：依赖 Mio 的位置预测。
- **其他模块联动**：SpeedMine（Rebreak/SwapReset/Alternative）、AntiCheat、AnchorAura、
  KillEffects、Freecam、Criticals。
- **Silent / Alternative / Pick 三种静默换手**：依赖 Mio 的 `FireworksHelper` 容器槽位操作，
  本移植只有 None / Normal（真实切换热栏槽位）。
- **Multipoint / Limit / LowerHitBox / HBFix / SmartTrace**：依赖 Mio 的
  `AutoCrystalSearchHelper4` 多点与命中盒判定。
- Mio 在垫方块前会对带菜单的方块按下潜行键（`is3051`），本移植未做。
- **Extrapolation（ExtraTicks）**：依赖 Mio 的位置预测，本移植按目标当前位置计算。
- **异步扫描**：Mio 用虚拟线程 + `CompletableFuture` 跑扫描。本移植同步扫描，
  先用原版放置条件做廉价过滤再算伤害，候选通常只有几十个。
- **DamageSync / Resync / Inhibit / Balance / Await / Instant / Sequential / StrictTiming /
  SwapDelay / SwapPenalty / TicksExisted / Weakness / Pause 组**：Mio 的时序微调与反检测调参。

## 验证状态

**未做运行时验证**（按要求没有反复启动客户端）。目前只有 `gradlew build` 通过。
建议需要时单跑一次：目标是确认实际会放置、伤害数值合理、切槽与转头顺序正确。
伤害模型可以脱离渲染单测，按本项目惯例新建 `build/autocrystal-validation/` 即可。
