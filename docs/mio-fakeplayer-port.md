# Mio FakePlayer 移植

参考 `skid/openmio/src/main/java/me/mioclient/Feature_14.java`。
实现位于 `src/dev/myth/model/misc/FakePlayer.java`，列在 Misc，默认关闭、未绑定。

Mio 把它做成 `.fakeplayer` 聊天命令（`Feature` 子类），本项目按要求改成模块：
开启即生成，关闭即移除，原来的 `record` / `play` 子命令变成两个开关。

| 设置 | 默认 | 说明 |
| --- | --- | --- |
| Name | `Herobrine` | 1～16 字符，修改后立即用新名字重新生成 |
| Record | false | 每个 MotionEvent POST 记录一帧；重新开启会清空上一次录制 |
| Play | false | 每个 MotionEvent PRE 循环回放一帧 |

## 保留的 Mio 行为

- 客户端实体固定使用 Mio 保留的 id `-9344`，生成在本地玩家位置，复制整个背包。
- 致命伤改为弹图腾：血量回到 9、清空药水、加 900 tick 再生 II、图腾粒子与音效。
- 每 80 tick 重新施加再生 II、抗性、防火、伤害吸收 IV，并把吸收量补到 16。
- 副手始终保持一个不死图腾（槽位 40）。
- 不可推动、免火、`isAlive` 恒为真、`deathTime` 永远清零，不进入死亡动画。
- 录制内容为位置、yaw、pitch、headYaw、速度、潜行和落地状态，回放时按 Mio 的三步插值。

## 26.2 适配

- `OtherClientPlayerEntity` → `RemotePlayer`；`updateTrackedPositionAndAngles` → `moveOrInterpolateTo`；
  `getInventory().clone` → `replaceWith`；`EntityPose` → `Pose`；`particleManager.addEmitter` →
  `particleEngine.createTrackingEmitter`。
- **伤害**：26.2 的 `hurtOrSimulate` 在客户端只走 `hurtClient`，不改血量，所以照搬 Mio 的
  攻击转发不会掉血。改为拦截发往 `-9344` 的 `ServerboundAttackPacket`：取消发送（这个实体
  服务端不存在，发出去就是一次无效攻击），同时在本地按原版公式结算伤害
  （`getAttributeValue(ATTACK_DAMAGE)` × `0.2 + scale² × 0.8`，先扣吸收）。
- **吸收**：26.2 的 `setAbsorptionAmount` 会被 `MAX_ABSORPTION` 属性夹住，而纯客户端实体
  收不到药水的属性修饰符，照搬会被夹成 0。生成时直接把该属性基础值设为 16，
  Mio 的 16 点护盾才成立。
- `ClientboundExplodePacket` 在 26.2 已不再携带受影响方块和逐实体伤害，
  Mio 的爆炸伤害模拟无法按原样移植，**未移植**。
- 本项目没有 Freecam 模块，Mio 的「开着 Freecam 时生成在摄像机位置」分支不适用。
- Mio 的 `DisconnectEvent` / `GameJoinS2CPacket` 清理，对应本项目的 `RespawnEvent`：
  旧世界已经销毁，直接丢弃旧实体并在新世界重新生成。

## 验证

`build/fakeplayer-validation/`，执行：

```powershell
.\gradlew.bat -I build/fakeplayer-validation/validation.gradle runFakePlayerValidation --console=plain --no-configuration-cache
```

独立 26.2 单人世界 36 项 PASS，覆盖：分类与默认值、生成位置与 id、名字与背包复制、
副手图腾、攻击包取消与本地掉血、真实实体不受影响、致命伤弹图腾、无死亡动画、
80 tick 效果循环与吸收补满、免火、录制帧数、回放区间与逐帧推进、重新录制清空、
改名重新生成、配置回环、关闭后实体移除与状态清理、渲染无 OpenGL 错误。
结果在 `build/fakeplayer-validation/result.txt`，截图 `fakeplayer.png`。
