# 黑屋排查交接文档

面向接手「Nova 在 HeyPixel 上被关黑屋」这个问题的 agent。这份只讲这一件事，
项目本身怎么构建、怎么注入、怎么调试看 `agent-handoff.md`。

**当前状态：问题未解决。已经排除掉一大半，剩下两个嫌疑点已定位到具体行，但还没改。**

---

## 1. 现象

在 HeyPixel 上跑 Nova，账号会被丢进影子池（黑屋）—— 匹配不到人、排不进对局。
判定方式是玩家自己报「黑屋 / 不黑屋」，**客户端这边看不到任何异常**。

### 关于这个判据的警告

`-- 不要把它当成可靠信号`

历史上同一个配置跑出过相反结果。它受排队时段、服务器人数、匹配延迟影响。
排得慢 ≠ 被检测。拿到任何单一结果都不要直接下结论，一个配置至少重复两三次。
（这条是从更早的会话里踩出来的，写在这里是为了别让下一个人重蹈覆辙。）

---

## 2. 当前代码状态（工作区里已经改了的东西）

这几处改动**已经进 DLL**，`build-vs2026/Release/NovaClientBase.dll`（Sep 11 13:40，TLS 守卫 = 0）
就是这一版。记牢现在是什么状态，别把它当成原始代码：

| 文件 | 状态 |
|---|---|
| `src/minecraft/modules/combat/killaura/KillAura.cpp:185-198` | `OnPlayerTick` 里 `java::AttackEntity()` 和 `java::SwingHand()` **已删除**。这一版 **KillAura 只瞄不打**（诊断用，不是修复） |
| `src/minecraft/modules/combat/aimassist/AimAssist.cpp:57-68` | `IsTeammate` 改成接收外部传入的 `selfName` |
| `src/minecraft/modules/combat/aimassist/AimAssist.cpp:253-255` | `selfName` 提到循环外，每 tick 只算一次 |
| `src/minecraft/modules/combat/aimassist/AimAssist.cpp:271-280` | 队友判断从「所有玩家」挪到**距离过滤之后** |
| `src/minecraft/hooks/Hooks.cpp` | `HookBodyYaw` / `HookRenderEntity` 受 `RenderRotationDisabled()` 门控 |

另外 **`build-vs2026/Release/nova-no-renderrot.flag` 还在**。
它会把本地转头通道（`HookRenderYaw` / `HookRenderPitch` / `HookBodyYaw`）整个关掉。
**目前所有「干净」的测试结果都是在它开着的情况下拿到的**，这一点始终是个未被排除的混杂因素。
做最终验证前要把它删掉。

拿到的日志：`C:\Users\13366\AppData\Roaming\.minecraft\nova-base.log`。

---

## 3. 已经排除的（别再查这些）

下面这些**都在某次「不黑屋」的运行里真的跑过**，所以可以排除：

- **所有旋转通道** —— 渲染通道、发包通道（`HookMotionYaw`/`HookMotionPitch`）、
  直接写字段（`SetPlayerRotation`）、GCD / 鼠标灵敏度量化。
  证据：`Ignore teammates` 关掉 + Range 4.5 + Speed 10 那次是干净的，
  而那条路径上 `SetRotations` / `SetPlayerRotation` **确实执行了**。
- **Nova 本身的加载** —— 10 个 retransform 的类、随机命名桥接类、`RegisterNatives`、
  24 个 native hook、GLFW swap hook。
- **渲染线程调 JNI** —— Sprint / AutoClicker 每帧都调，干净。
- **`WorldPlayers` + 每玩家的 `DeathTime` / `Health` / `IsInvisible`（+ `IsInstanceOf`）** —— 都在干净那次跑过。
- **`WorldEntities` + 每实体的 `EntityTypes`（8 次 `IsInstanceOf`）/ `IsSame` / `IsSpectator` / `IsAlive`** ——
  在 KillAura `Player` 关掉那次跑过。

注意最后两条很重要：**「透视类」方法本身不是雷**。`getHealth`、`isInvisible`、`getX` 全跑过，没事。

---

## 4. 测试矩阵

| # | AimAssist | KillAura | 结果 |
|---|---|---|---|
| 1 | 关 | 关 | 干净 |
| 2 | `Weapon only` 开 + 手持非武器 | — | 干净 |
| 3 | `Ignore teammates` 关，Range 0 | — | 干净 |
| 4 | `Ignore teammates` 关，Range 4.5，Speed 10 | — | 干净 |
| 5 | `Ignore teammates` 开，Range 0 | — | **黑屋** |
| 6 | `Ignore teammates` 开，Range 4.5 | — | **黑屋** |
| 7 | `Ignore teammates` 开（完成 §2 的修复后） | — | **黑屋** |
| 8 | — | `Player` 关 | 干净 |
| 9 | — | `Player` 开（只转头不打人） | **黑屋** |

第 2 行说明 `GetEnemy()` 根本不跑的时候是干净的 —— 触发点在 `GetEnemy()` 里面。

---

## 5. 剩下两个嫌疑点

### 5.1 AimAssist —— 队友分支

发生在 `AimAssist.cpp` 的 `GetEnemy()` 里。开关 `Ignore teammates` 是唯一变量，
ON/OFF 的对照重复了三次，方向一致。

第 3 行（Range 0 + OFF，干净）和第 5 行（Range 0 + ON，黑屋）是关键对照：
**Range = 0 时旋转一次都没跑，唯一差别就是队友分支。**

做完 §2 的修复后，Range = 0 时队友分支只剩这一句会执行
（`IsTeammate` 在距离过滤之后，Range 0 时永远到不了）：

```cpp
// AimAssist.cpp:253-255
std::string selfName;
if (ignoreTeammates)
    self.DisplayName(selfName);      // ← 每 tick 一次
```

而 `DisplayName` 这条链（`GameEntity.cpp:1016-1048`）是：

```
Entity.getDisplayName()      GameEntity.cpp:1022   (SRG m_5446_)
  → Component.getString()    GameEntity.cpp:1029
    → GetStringUTFChars      GameEntity.cpp:1037   ★
```

**★ 这一句是全项目唯一的 `GetStringUTFChars`**（grep 过：另一处 `Env.cpp:106` 只在
Java 抛异常时打日志走，正常运行不执行）。

Java 侧我们读过源码（`skid/MCP-Reborn-1.20.1-official`），这三个方法全是纯本地计算，
不碰网络、不发包、不写任何服务端可见状态：

```java
// Entity.java:2192
public Team getTeam() { return this.level().getScoreboard().getPlayersTeam(this.getScoreboardName()); }
// Entity.java:2196
public boolean isAlliedTo(Entity e) { return this.isAlliedTo(e.getTeam()); }
// Entity.java:2572
public Component getDisplayName() {
    return PlayerTeam.formatNameForTeam(this.getTeam(), this.getName())
        .withStyle(s -> s.withHoverEvent(this.createHoverEvent()).withInsertion(this.getStringUUID()));
}
```

所以如果真是这几行触发的，**触发点只能是「调了它们」这件事本身**，不是它们做了什么。
再结合第 3 节那条「`getHealth` / `isInvisible` 同类方法都干净」，唯一跟它们不一样的就是
**「把 Java 字符串导成 UTF-8 到 C++」这一步**。这是推测，不是结论。

第二嫌疑是 `IsAlliedTo`（`AimAssist.cpp:60` → `GameEntity.cpp:1001`）。
Range 0 那次它没跑，所以它排在上面那条后面。

### 5.2 KillAura —— `Player` 开着时多出来的调用

`Player` 关掉时 `targets_` 是空的，`OnGameTick` 会提前 return，
所以下面这些**只在 `Player` 开着时跑过**：

| 位置 | 调用 |
|---|---|
| `KillAura.cpp:153` | `rotation::RayCast(calculate, range)` → `Level.clip(...)` |
| `KillAura.cpp:168` | `java::AttackTargetEntity` → 读 `Minecraft.hitResult` + `EntityHitResult.getEntity()` |
| `KillAura.cpp:176` | `hovered.HurtTime()` |
| `KillAura.cpp:179` | `java::PlayerHurtTime()` |
| `KillAura.cpp:283` | `HoveredIsPlayer()` 里又一次 `AttackTargetEntity` |

**下一步不用改代码就能分辨：把 `Hit select` 关掉，`Player` 保持开。**

`hitSel_` 是 `AttackTargetEntity` / `HurtTime` / `PlayerHurtTime` 的总闸
（`KillAura.cpp:106` 和 `:164` 两处都在 `hitSel_` 后面），一关就只剩 `RayCast` + `SetRotations`：

- 干净 → 是 `AttackTargetEntity` 那条
- 还黑屋 → 是 `RayCast`（`Level.clip`）

---

## 6. 提过的改法（都还没动手）

按用户要求：**AimAssist 的队友功能不能直接删。**

### 方案 A —— 把前缀比较搬进 Java

- **改哪两个文件**：`src/minecraft/java/game/GameEntity.cpp`（加一个方法，如
  `DisplayNameSharesPrefixWith(other, prefixLen)`）、`src/minecraft/modules/combat/aimassist/AimAssist.cpp`（`IsTeammate` 改调它）
- **干什么**：现在是 `getDisplayName()` → `getString()` → **`GetStringUTFChars`** → C++ 里比 `std::string`；
  改成 `getDisplayName()` → `getString()` → Java 里 `substring(0,2)` + `startsWith()` → 只回 `boolean`
- **判断结果完全一样**，`Ignore teammates` 的行为一点不变，只是 `GetStringUTFChars` 这一环没了
- **影响面**：只碰 AimAssist 的 `Ignore teammates`，约 40 行

### 方案 B —— 只缓存 `selfName`

1 秒刷一次（名字本来就不会变），把每 tick 一次的调用降到每秒一次。
改动最小，但如果是「调了就中」的硬触发器，这个不管用。

§5.1 的分析偏向 A，但 A 没有排除 `getDisplayName` / `getString` 本身。
如果 A 做完还黑屋，下一步就是把 `getDisplayName` 也搬进 Java ——
**注意桥接类规则**：不做批量 Java 注入，每个桥接一个随机命名的生成类，逻辑留在 C++。

---

## 7. 约束（别破坏）

- **改源码前先列清楚**：改哪几个文件 / 为什么 / 影响面多大 / 有没有更小的改法，等用户同意再动手。
  构建可以直接做，不用问。
- **反射加载**：DLL 里不能有 `thread_local` / `__declspec(thread)`；函数内 `static` 不能带动态初始化；
  卸载只能走 `VirtualFree`。验收：`dumpbin //disasm ... | grep -c "gs:0000000000000058h"` 等于 0。
- **`src/core/ReflectiveLoader.c:182` 的 `PAGE_EXECUTE_READWRITE`** 用户明确选择不动。
- **不用匹配延迟当检测判据**（见 §1）。

## 8. 事实速查

| 项 | 值 |
|---|---|
| 游戏 | Minecraft 1.20.1 Forge 47.4.23，运行时 SRG 命名（`m_12345_` / `f_12345_`） |
| 反编译源码 | `skid/MCP-Reborn-1.20.1-official`（官方命名，找 Java 侧实现看这里） |
| SRG 常量 | `src/minecraft/java/game/GameEntity.cpp:60-95`（`kIsAlliedTo = "m_7307_"`、`kGetDisplayName = "m_5446_"`） |
| 日志 | `C:\Users\13366\AppData\Roaming\.minecraft\nova-base.log` |
| 产物 | `Client/build-vs2026/Release/NovaClientBase.dll` |
| 卸载 | 游戏里按 `DEL`（`LoadLibraryA` 加载已加载的 DLL 不会重跑 DllMain） |
