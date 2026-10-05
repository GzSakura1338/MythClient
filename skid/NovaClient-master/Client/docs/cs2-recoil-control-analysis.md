# CS2 Recoil Control：武器数据与模块设计

日期：2026-09-22。阶段：只读分析与设计；尚未编写 RCS 模块、安装输入 Hook 或操作游戏视角。本次未修改生产代码、未构建或注入 DLL。

补充分析后的最终行为、输入时序约定与验收标准见 [Recoil Control 设计文档](cs2-recoil-control-design.md)。下文保留第一轮证据；设计文档中补充了生成表、射击合成求值、落地扰动和输入历史的调用链。

## 数据来源与范围

IDA 分析对象为 Build 14181 的 client.dll 副本，与本机已安装文件 SHA256 一致：

`A0C195F0B6EC00915EF08C548200A010EBBE7982D3A4BC468CAD939B67C8C4E3`

原文件：`D:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Global Offensive/game/csgo/bin/win64/client.dll`。

通过本机 Source2Viewer CLI 从当前 `pak01_dir.vpk` 重新导出 `scripts/weapons.vdata_c` 和 `scripts/items/items_game.txt`。解码结果与此前分析目录中的两个文件哈希一致。武器数据使用数值 Item Definition Index 条目，并与 items_game 中的名称逐项核对，避免把 prefab、别名、皮肤或物品实体重复计数。

数据目录：[20260922-build14181-weapons](../../skid/cs2-dumps/20260922-build14181-weapons/)。

| 文件 | 内容 |
| --- | --- |
| [weapon-summary.csv](../../skid/cs2-dumps/20260922-build14181-weapons/weapon-summary.csv) | 35 条武器摘要：编号、类别、弹匣、伤害、射击间隔、后坐力和种子等 |
| [weapon-fields.json](../../skid/cs2-dumps/20260922-build14181-weapons/weapon-fields.json) | 每把枪的直接标量、数组属性；保留原字段名称 |
| [weapons-firearms.kv3](../../skid/cs2-dumps/20260922-build14181-weapons/weapons-firearms.kv3) | 35 条完整资源块，包含嵌套音效、分类等信息 |
| [scripts/weapons.vdata](../../skid/cs2-dumps/20260922-build14181-weapons/scripts/weapons.vdata) | 完整解码武器资源，包含基础类型、prefab、非枪械与别名 |
| [scripts/items/items_game.txt](../../skid/cs2-dumps/20260922-build14181-weapons/scripts/items/items_game.txt) | 完整经济系统物品定义 |
| [ida-evidence.json](../../skid/cs2-dumps/20260922-build14181-weapons/ida-evidence.json) | 本次 IDA 反编译证据及关键汇编 |
| [prior-live-comparison.json](../../skid/cs2-dumps/20260922-build14181-weapons/prior-live-comparison.json) | 与此前场景 dump 的逐项比较 |
| [manifest.json](../../skid/cs2-dumps/20260922-build14181-weapons/manifest.json) | 二进制、VPK 目录、资源与导出文件指纹 |

覆盖全部 34 把常规枪械，另保留 Zeus，共 35 条。此处“完整”指安装资源内的枪械配置；不代表已采集每把枪在服务器上的实时状态、逐发弹道或随机散布结果。

| 类别 | 数量 | 物品编号与武器 |
| --- | ---: | --- |
| 手枪 | 10 | 1 Deagle、2 Dual Berettas、3 Five-SeveN、4 Glock、30 Tec-9、32 P2000、36 P250、61 USP-S、63 CZ75、64 R8 |
| 步枪 | 7 | 7 AK-47、8 AUG、10 FAMAS、13 Galil、16 M4A4、39 SG553、60 M4A1-S |
| 冲锋枪 | 7 | 17 MAC-10、19 P90、23 MP5-SD、24 UMP-45、26 Bizon、33 MP7、34 MP9 |
| 狙击枪 | 4 | 9 AWP、11 G3SG1、38 SCAR-20、40 SSG08 |
| 霰弹枪 | 4 | 25 XM1014、27 MAG-7、29 Sawed-Off、35 Nova |
| 机枪 | 2 | 14 M249、28 Negev |
| 电击枪 | 1 | 31 Zeus；保留数据，RCS 不处理 |

数组顺序为主/副射击模式。副模式的具体含义由枪种和当前状态决定，不能统一解释为“右键开镜”。CSV 的 NominalRPM 仅为主模式 `60 / CycleTime` 的派生值，不计装填、拉栓、连发间隙等限制；Zeus 的该值不代表实际持续射速。

## IDA 已确认的结构

以下地址为 client.dll RVA，适用本次指纹。IDA 自动推导的函数签名存在遗漏参数，不能直接复制为调用声明。

| RVA | 观察结果 |
| --- | --- |
| `0x7F2FD0` | CCSWeaponBaseVData 的字段读取代码，确认 CycleTime、RecoilAngle、Magnitude、Variance、Seed 等字段目标偏移 |
| `0x7D4A70` → `0x18A8390` | CFiringModeFloat 读入两个值；资源为单值时补齐后续模式值。因此标量 0.1 不能解释为副模式 0 |
| `0x7AA490` → `0x11125A0` | 从 EconItemView 的物品定义取得数值编号，再以编号查找武器 VData |
| `0x8A43F0` | 通过 Pawn `+0x14B8` 获取 AimPunchServices，转发输出指针及 R8 参数 |
| `0x82B5D0` | 分别求值 predictable / unpredictable 两个分量，再相加输出 |
| `0x82B780` | 取得与 Pawn/预测上下文有关的时间基准 |
| `0x827220` | 根据时间衰减、缓存并插值角度；观察到 128 步时间离散，不代表服务器 tickrate 为 128 |
| `0x8DCC80` | 读取基础角度，调用上述包装函数，再叠加后坐力结果；调用点 `0x8DCCC6` 明确设置 `R8B=1` |

`0x827220` 的末尾存在由布尔参数控制的角度倍增。调用有效角度接口时必须确认该参数及调用链，不能拿已经缩放的结果再固定乘 2。资源中的 RecoilMagnitude 也不是每帧应直接减去的视角角度。

本版本服务的两组基础值、角速度和时间基准共同参与求值。只读 `m_predictableBaseAngle` 会漏掉随时间变化的部分；把旧版本的 `m_aimPunchAngle`、`m_aimPunchCache` 偏移照搬也不适用。

### 字段清单

这些偏移来自现有 Build 14181 Schema dump；上面的反编译进一步确认了后坐力服务入口和 VData 后坐力字段。并非每个字段都已进行射击时的动态验证。

| 所属结构 | 字段 | 偏移 |
| --- | --- | --- |
| C_BasePlayerPawn | m_pWeaponServices | `0x1208` |
| CPlayer_WeaponServices | m_hActiveWeapon | `0x60` |
| C_EconEntity | m_AttributeManager | `0x11A8` |
| C_AttributeContainer | m_Item | `0x50` |
| C_EconItemView | m_iItemDefinitionIndex | `0x1BA`，uint16 |
| C_CSPlayerPawn | m_pAimPunchServices / m_iShotsFired | `0x14B8` / `0x1C8C` |
| CCSPlayer_AimPunchServices | predictable tick / tick fraction | `0x48` / `0x4C` |
| CCSPlayer_AimPunchServices | predictable angle / angular velocity | `0x50` / `0x5C` |
| CCSPlayer_AimPunchServices | unpredictable tick / angle | `0xA0` / `0xA4` |
| C_CSWeaponBase | weaponMode / integer recoil index / float recoil index | `0x17D8` / `0x17FC` / `0x1800` |
| C_CSWeaponBase | m_bInReload | `0x1814` |
| C_BasePlayerWeapon | next primary tick / fraction / clip1 | `0x16F0` / `0x16F4` / `0x1700` |
| CCSWeaponBaseVData | weapon type / full auto / cycle time | `0x520` / `0x734` / `0x740` |
| CCSWeaponBaseVData | angle / angle variance | `0x798` / `0x7A0` |
| CCSWeaponBaseVData | magnitude / magnitude variance / recoil seed | `0x7A8` / `0x7B0` / `0x7DC` |

活动武器必须通过完整句柄和序号校验解析，不能仅取低位索引；实体及服务指针不跨会话缓存。

## 已发现的旧 dump 限制

旧 `sdk/weapons/weapons.json` 只包含当时场景内 13 个实体名称，混有刀具和投掷物。它的采集代码 `skid/cs2-universal-offsets/src/analysis/weapons.rs` 使用 identity 的 designer name 去重，没有以物品编号区分型号。

将其中 7 条枪械记录的 14 项数值与资源比较，共 98 项：5 把可按原名称匹配；另两条全部 14 项数值分别匹配下面的资源型号：

- 标为 `weapon_m4a1` 的记录匹配 ID 60 M4A1-S，不匹配 ID 16 M4A4。
- 标为 `weapon_hkp2000` 的记录匹配 ID 61 USP-S，不匹配 ID 32 P2000。

这是已有数据与采集方法的证据，不是本次重新读取这两个实体的运行时编号。后续模块的唯一型号键必须是 Item Definition Index，名称只用于显示。不能将旧 dump 当成完整或严格按型号区分的武器表。

## 建议的模块设计

模块名称 `Recoil Control`，Combat → General，默认关闭，接入现有配置、绑定、ArrayList 和模块生命周期。保持原生 C++ 实现。

第一版采用实时后坐力反馈：在同一输入/预测时序取得有效后坐力，按水平和垂直强度计算补偿变化，并与用户原始鼠标输入合成。不按 FPS 累加固定角度，不依赖固定枪械轨迹，不重写随机散布。

参数保持四项：

| 参数 | 范围 | 建议初值 | 作用 |
| --- | --- | --- | --- |
| Horizontal | 0–100% | 100% | 水平后坐力补偿比例 |
| Vertical | 0–100% | 100% | 垂直后坐力补偿比例 |
| Start Bullet | 1–10 | 2 | 本轮射击从第几发开始；实际开火状态驱动 |
| Smooth | 0–100 ms | 0 ms | 0 为直接补偿；非零按实际时间平滑，延迟会降低瞬时补偿效果 |

以上是待测试的设计初值，不是已经验证的最佳实战参数。暂不为 35 把武器各堆一组参数；运行时自动识别武器和模式。Zeus、刀具、投掷物、C4、非本地或旁观 Pawn 不处理。

### 状态与时序

1. SDK 负责版本校验、活动武器快照、物品编号、射击/装填状态及后坐力求值；未知版本时停用 RCS。
2. 输入 Hook 负责在确定的游戏输入时序运行。同一命令不能重复补偿，避免重预测、多次回调或高 FPS 叠加。现有 Present/模块 onUpdate 仅同步设置，不能直接修改视角。
3. 模块负责强度、起始发数和平滑策略，只校正本地射击后坐力，不寻找目标或自动开火。
4. 切枪、重生、断线、进入旁观、失焦、打开菜单、停止射击和关闭模块时清理上一轮状态；开启中途以当前有效状态建立基线，避免套用上一把枪的历史量。松开扳机后不主动把准星拉回开火前的位置。
5. 连发枪的有效射击可能持续到松开按钮之后，不能只依据鼠标左键判断。装填中、空弹匣但没有新射击、特殊副射击状态分别处理。

拟定目录：`cs2/sdk/weapons/` 放数据访问，`cs2/hooks/` 放输入时序适配，`cs2/modules/combat/recoilcontrol/` 集中模块与状态。公共 UI 仅调用现有属性控件，不引入 CS2 偏移或游戏对象。

### 实施前仍需验证

- 输入 Hook 的准确调用时序、ABI 和预测重入语义。已有 dump 中标记的 CreateMove 只能作为候选，尚未验证为可直接接入的位置。
- 后坐力求值接口的所有参数和线程要求；反编译显示其会维护内部缓存，不应在任意渲染线程调用。
- 补充分析已确认射击准备路径 `0x7E8600` 使用 `0x82B500` 在给定时间合成两个分量，且落地路径也会修改 predictable。设计选择射击活动期间的有效合成角度反馈；运行时仍需验证调用时序，不将 predictable 当成纯枪械来源。
- 逐枪验证主/副模式切换、消音器、连发、R8、点射、长喷、装填和切枪。静态资源完备不等于这些运行行为全部验收。
- 独立逻辑测试覆盖重复命令、停射重开、异常角度、失效实体、平滑在不同帧率的时间一致性。游戏实测由用户完成。

结论：数据足以确定型号识别、字段布局和 RCS 架构；输入接入及有效角度选择还需在实施阶段完成验证，当前没有声称模块已可用。
