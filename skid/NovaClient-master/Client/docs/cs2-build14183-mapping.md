# CS2 Build 14183 mapping

2026-09-24，Steam Build ID `25492732`。本次只迁移现有功能的版本绑定，不实现第三人称。

## 数据来源

本机完整导出：`skid/cs2-dumps/20260924-steam25492732-full`。a2x 和 cs2-sdk 均成功退出；SDK 找到 424/503 个通用签名，不能据此声称所有签名均有效。生产代码不依赖这些导出文件。

已将 client、engine2、tier0、resourcesystem、materialsystem2、scenesystem 的安装文件 SHA256 与导出指纹逐一核对。生产 SDK 保留严格的镜像大小、哈希及函数入口字节校验。

## 原生入口

以下地址均为对应模块 RVA，来自新旧 DLL 的 IDA 交叉引用、反编译及 PE 字节核对；不是统一加偏移。

| 用途 | Build 14183 |
| --- | --- |
| client 实体系统 / 本地控制器 / 本地 Pawn | `2711048 / 25334D8 / 255C5A8` |
| client 视图矩阵 | `25618F0` |
| engine2 build / network client | `61C1EC / 91A150` |
| client 输入对象指针 / globals / 输入抑制 | `22343C0 / 2227F08 / 21F8010` |
| 输入帧 / 最终角度提交 / 提交返回地址 | `B7B010 / BEEEE0 / B7B4B8` |
| AimPunch 时间 / 插值结果 | `851BD0 / 851950` |
| inaccuracy / spread | `80C310 / 80D600` |
| 命中盒集 / 变换 / 骨骼查找 | `93F860 / 9459D0 / 951950` |
| 射击起点 / trace / trace manager | `A7E930 / 826310 / 221F1E8` |
| world FOV / base FOV | `889600 / A66E30` |
| OverrideView / 调用返回地址 / ClientMode 主虚表 | `D015F0 / B9B807 / 1C99980` |
| Radar 玩家更新 / 显示标志 | `EB8610 / +17808` |
| Time powf 调用 / 返回地址 / IAT | `11F0984 / 11F098A / 1AA6538` |
| Pawn 主虚表 | `1C83E58` |
| scenesystem 绘制 | `59FC0` |
| client KV3 构造 / tier0 LoadKV3 / KV3 清理 | `19ADAB0 / 124810 / 21B620` |
| materialsystem2 创建材质 | `3ABF0` |
| resourcesystem 查询 / 删除 | `14BA0 / 17AC0` |
| ResourceSystem013 / ResourceHandleUtils001 虚表 | `65348 / 65868` |

## 布局与调用链

- Schema 字段分别迁移：控制器 Pawn `92C`、名称 `878`、护甲 `93C`、ping `838`；Pawn 护甲 `1ECC`、武器服务 `12F0`、相机服务 `1328`、AimPunch 服务 `1598`、开镜 `1EA0`、射击次数 `1EB4`、免疫 `3508`。武器属性容器 `1290`、弹夹 `1928`、模式 `1A00`、recoil index `1A28` 等以导出 Schema 为准。
- 输入采样数 `B50`、数组 `B58`、步长 `440`、角度 `430` 经新输入函数核对保持不变。最终提交函数写 Pawn `13A8`，FOV 去抖读取同步迁移。第三人称现有只读标志仍为 input `229`。
- 新 ClientMode 插入了一个虚函数：OverrideView 从虚表槽 15 变为 16。新调用点是 `B9B801: call [rax+80h]`，必须同时迁移返回地址和六字节校验。View FOV `498`、angles `4B8` 及 mode slot `28` 保持不变。
- world/base FOV 的服务虚表偏移仍为 `118/120`；武器服务射击起点槽仍为 `100`。同名导出 `GetTransformsForHitboxList` 不是 Nova 当前调用的四参数 helper，使用已验证的 `9459D0`。
- Trace 结果仍为 192 字节，命中实体 `+8`、fraction `+AC`、solid `+BB/+BC`。命中盒字段和 `70` 步长保持不变。
- engine2 `69CA0` 的 ProcessTick 写最新服务器 tick 到 `+39C`，旧 `+37C` 不再使用。`6A250` 的 ProcessServerInfo 仍使用 server count `244`、connection `168`、address `28`。
- Chams 仍为七参数绘制、记录步长 `70`、scene `18`、material `20`、color `50`；资源引用计数仍在 binding `+32`。场景 owner `B8` 延续现有实现，未作游戏内独立验收。

## 验证边界

38 个生产 Schema 字段及 19 处生产入口/调用字节与新版本逐一匹配；六个模块指纹与安装文件一致。Release DLL、NovaCs2Smoke 构建成功，完整 CS2 离线回归和 DLL 静态 TLS 检查通过。日志位于 `Client/build-vs2026/cs2-14183-build.log` 和 `cs2-14183-smoke.log`。

离线回归验证逻辑、Hook 生命周期和渲染测试，不代表游戏内验收。验证阶段 CS2 已退出，未注入或自动启动游戏。`check-client-layout.py` 仍报告迁移前已有的断链：`cs2-third-person-analysis.md` 引用不存在的 `cs2-third-person-design.md`；与本次 mapping 无关。
