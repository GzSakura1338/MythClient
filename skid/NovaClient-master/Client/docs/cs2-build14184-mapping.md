# CS2 Build 14184 mapping

2026-09-25，Steam Build ID `25515854`。更新现有 CS2 功能的版本绑定，保留 client/engine 指纹、构建号及函数入口字节校验。

## 数据来源

- 正在运行的游戏 PID `42752`；完整导出目录 `skid/cs2-dumps/20260925-steam25515854-full`。
- a2x / cs2-sdk 均成功退出，29 项全局偏移一致；SDK 匹配 424/503 个通用签名。未匹配项不作为生产地址来源。
- `Client/build-vs2026/analysis/cs2-client-build14184.dll` 和 `cs2-engine2-build14184.dll` 是本次安装文件副本；SHA256 与运行时导出记录一致。
- 通过旧版 IDA 函数边界、新旧 PE 反汇编、RIP 相对引用和 MSVC RTTI 核对函数及数据地址。17 个关键函数在屏蔽相对地址引用后，未发现指令字节变化；没有按统一差值平移地址。

## 更新地址

下表都是对应模块 RVA。

| 用途 | Build 14184 |
| --- | --- |
| client 实体系统 / 本地控制器 / Pawn | `27130E8 / 2535598 / 255E658` |
| client 视图矩阵 | `25639A0` |
| engine2 build / network client | `61D1E8 / 91B1C0` |
| 输入对象指针 / globals / 输入抑制 | `2236440 / 2229F88 / 21FA030` |
| 输入帧 / 角度提交 / 提交返回地址 | `B7C070 / BEFF40 / B7C518` |
| AimPunch 时间 / 插值 | `852C10 / 852990` |
| inaccuracy / spread | `80D1E0 / 80E4D0` |
| 命中盒集 / 变换 / 骨骼查找 | `9408B0 / 946A20 / 9529A0` |
| 射击起点 / trace / trace manager | `A7F980 / 827320 / 2221248` |
| Auto Fire 过滤器 / 世界射线 / 玩家补充检测 | `3520D0 / 2006D0 / AC9CA0` |
| world FOV / base FOV | `88A640 / A67E80` |
| 最终视角 setup / OverrideView / 调用返回地址 | `B9C5D0 / D02650 / B9C867` |
| ClientMode 主虚表 / Pawn 主虚表 | `1C9AB90 / 1C85068` |
| cinematic camera / CDataDrivenCamera RTTI 类型 | `25764A8 / 22F42C0` |
| Radar 玩家更新 / HUD 标志 | `EB96E0 / +17808` |
| Time 场景颜色 powf 调用 / 返回地址 / IAT | `11E4266 / 11E426C / 1AA7538` |
| client KV3 构造 | `19AEB90` |

## 字段和校验

- `C_CSWeaponBase::m_fLastShotTime` 从 `1B60` 改为 `1B68`；`C_CSWeaponBaseGun::m_iBurstShotsRemaining` 从 `1F14` 改为 `1F24`。其他现用玩家/武器 Schema 字段经导出核对保持不变。
- 输入采样数 `B50`、数组 `B58`、步长 `440`、角度 `430` 和 Pawn 输入角度 `13A8` 沿用对应函数中的布局。
- ClientMode 的 OverrideView 仍为虚表槽 16；最终视角 setup Hook 保持现有逻辑，FOV 不接管结算镜头。镜头/视图结构偏移未变化。
- FOV world getter 内的调用位移变化，入口校验字节同步更新。Time 使用当前正常场景颜色通路的 `V_powf` 调用点和 IAT，完整调用字节同步更新。
- engine2 ProcessTick / ProcessServerInfo 分别迁移到 `69CD0 / 6A280`；HUD 仍读取服务器 tick `39C`，不使用 dumper 的 deltaTick `24C` 代替。
- Chams 使用新版 Pawn 虚表和 KV3 构造地址；scenesystem、materialsystem2、tier0、resourcesystem 的哈希和镜像大小与上一版一致，相关入口沿用已验证值。

## 验证记录

- `analysis/verify-build14184.py` 对生产常量执行 96 项检查，包括实际 Schema、全局偏移、运行时入口/调用字节、相对调用目标、RTTI、模块哈希与镜像大小。结果保存为 `analysis/build14184-production-audit.json`。
- 函数匹配及逐指令对比证据：`analysis/build14184-function-matches.json`、`analysis/build14184-full-function-comparison.txt`。新版 IDA 对输入帧、角度提交、Radar、OverrideView 的反编译记录保存为 `analysis/build14184-ida-evidence.json`。
- Release DLL 和 NovaCs2Smoke 构建成功；完整 CS2 离线回归退出码为 0，静态 TLS 存储及回调检查通过。日志：`Client/build-vs2026/cs2-14184-build.log`、`cs2-14184-smoke.log`、`cs2-14184-loader.log`。独立测试进程未加载游戏模块，宿主初始化阶段记录的 binding unavailable 是预期的版本保护行为。
- 产物 `Client/build-vs2026/Release/NovaClientBase.dll`：22,046,208 字节，SHA256 `2592B8C317EC6990DE5B8783E5C9A2638F610A417E76AA05EBA9ACE565B39EC4`。
- 不自动注入游戏；离线检查和回归不代表游戏内验收。
