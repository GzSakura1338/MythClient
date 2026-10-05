# Nova yaw 连续性修复

依据本地 `skid/Grim-2.0/common/src/main/java/ac/grim/grimac/checks/impl/aim/AimModulo360.java`
及 `utils/anticheat/update/RotationUpdate.java`：deltaYaw 为原始 `newYaw - oldYaw`，
在没有传送/载具等豁免时，若新 yaw 位于 (-360, 360)、本次差值绝对值大于 320、
上次差值绝对值小于 30，则触发检查。

原 Myth 的退出路径 `resetRotation(applySensitivityPatch(camera, lastRotations))`
没有将相机搬到静默 yaw 的同一圈数：例如连续输出 360.5、361 后，相机恢复为 1，
下一包出现 -360 的差值。`onPre` 还在更新 lastRotations 之前执行退出修正。
独立运行验证复现了旧退出路径满足此检查的条件。

参考 Nova：

- `Client/src/minecraft/utils/rotation/RotationManager.cpp`：`Smooth`、`QuantizeRequest`、
  `OnMotionPre`、`SetActive`、`CorrectDisabledRotations`、`OnRespawn`。
- `Client/src/minecraft/utils/rotation/RotationUtils.cpp`：`RelateToPlayerRotation`、`Move`、`Smooth`。
- `Client/src/minecraft/java/game/GameFreeLook.cpp`：`SetPlayerCameraRotation`。
- `Client/src/minecraft/utils/rotation/LinearAim.h`：相机及手部插值端点的整圈平移。

修订只接入现有共享 `RotationManager` / `RotationUtils`：

| 位置 | 修订 |
| --- | --- |
| 目标及随机射线偏移 | 将目标置于上一输出角度的最近一圈；fallback 使用已对齐目标 |
| 最终灵敏度取整 | 以 lastRotations 为原点，使用确定性鼠标步长，并保留同一圈数 |
| smooth 重载 | 使用调用者传入的 lastRotation；零速度仍取最近 yaw；完全重合时返回零步进 |
| PRE Motion 退出 | 先记录本次实际输出，再按最短角差判断完成并修正相机 |
| 退出静默转头 | 仅将相机 yaw 平移整数个 360 度，保持实时视角和 pitch；同步 yRotO、yBob、yBobO |
| 非法值及重生 | 拒绝非有限请求和输出；重生直接清理旧状态，不把旧世界圈数套到新玩家 |

没有新增功能模块或另一套移动修复；MoveUtils 的算法及事件优先级保留。
这些变更是用户要求的 Nova 修订，不能再宣称两类与最初 Starain 全文完全一致。
`scripts/verify-starain-rotation.ps1` 对其余 5 类继续逐行比较，
对这 2 类固定参考版本和已审核修订版的 SHA-256（仅统一行尾），防止未列出的后续变化。

独立 26.2 单人世界：56 项运行检查通过，包括原 MoveFix 的 4,350 组输入，
正反整圈转头、不同正负圈数、返回/退出/重新接管，以及相机/手部各插值点。
实际发送路径采集 400 个运动包，均满足上述本地检查并保持每次 yaw 差值不超过 180.2 度。
结果位于 `build/rotation-validation/rotation-result.txt`，验证代码不会进入正式 JAR。
这些结果覆盖本地复现场景，不代表已在用户所连服务器上验证所有检查。
