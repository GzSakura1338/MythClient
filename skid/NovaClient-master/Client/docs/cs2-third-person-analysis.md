# CS2 第三人称背视：逆向记录

状态：静态逆向完成；功能尚未实现，未注入游戏验证。日期：2026-09-23。对应[设计与验收方案](cs2-third-person-design.md)。

## 1. 样本与证据约定

| 项目 | 值 |
| --- | --- |
| 游戏构建 | Build 14181 |
| 分析样本 | `Client/build-vs2026/analysis/cs2-client-build14181.dll` |
| client.dll SHA256 | `A0C195F0B6EC00915EF08C548200A010EBBE7982D3A4BC468CAD939B67C8C4E3` |
| engine2.dll SHA256 | `7A72CBB4B9239309627DD52F029D72A7A5B09BFB36A3F0B9AF4411EB5F337E80` |
| PE SizeOfImage | client：41803776；engine：9842688 |
| IDA 映像基址 | `0x180000000` |

分析副本与本机已安装 client.dll 的 SHA256 一致。版本基线来自 [players/Offsets.h](../src/cs2/sdk/players/Offsets.h)。

**本文所有函数、虚表和全局地址均为 client.dll 的 RVA**，运行时加实际模块基址；对象字段和虚表槽使用字节偏移，不是槽序号。下文函数用途是依据反编译、交叉引用、RTTI 和指令推导的描述，不冒充官方符号。

原始证据保存在本机 `Client/build-vs2026/analysis/third-person/ida-evidence.json`，共 70 项；属于被 Git 忽略的分析产物。本文保留可独立复核的关键地址、字段和调用点。JSON 中地址为 IDA VA；反编译器推断的类型、数组长度和返回值必须用实际指令复核，不能直接复制为 C++ SDK。

## 2. 原生开关与镜头偏移

| RVA | 已确认的行为 |
| --- | --- |
| `0xB1E870` | 返回 `input + 0x229 + slot * 0x928` 的第三人称标记 |
| `0xB1E820` | 复制 `input + 0x230 + slot * 0x928` 的三个 float |
| `0xB20800` | 原生切入第三人称；仅 slot 0，设标记、复制角度、初始化距离 30，并调用 Pawn 虚函数 |
| `0xB20720` | 原生退出；除清标记外，还改动 shoulder、distance 等状态/cvar |
| `0xB1EB00` | 原生第三人称更新：权限检查、鼠标/距离控制、平滑、碰撞和最终偏移 |
| `0xB53E90` | 在视图中应用第三人称位移；支持肩位和对应角度处理 |
| `0x20C54E8` | input 对象指针的全局存放位置 |

三个 float 是 **pitch、yaw、distance**，不是世界坐标 XYZ。`0xB53E90` 读取它们，将眼位减去方向向量乘距离；它还调用 Pawn 虚表 `+0xA10`，不能把整个函数当成无副作用的位移工具。

`0xB1EB00` 对每个 slot 使用 `input + 0x228 + slot * 0x928` 子状态，存在特殊相机分支，并在 `sv_cheats` 条件不满足时清除第三人称标记。因此一次写入该标记不能构成稳定实现。设计不采用修改 `sv_cheats`、反复强制标记或调用原生退出命令恢复状态的方案。

## 3. 相机、缓存与投影顺序

### 3.1 实际调用链

普通玩家路径的关键顺序：

1. `0xBC6880` 调用 `0xBCC420` 构建视图。
2. `0xBCC420` 调用相机服务虚表 `+0x108`，输出眼位、角度、裁剪面及 FOV。
3. `0xBCC777` 调用 ClientMode 虚表 `+0x78`，实际入口 `0xCB2930`。
4. `0xCB2930` 先调用 `0xB53E90`，再处理固定/调试相机或 data-driven 相机。
5. 回到 `0xBCC420`，继续处理其他特殊分支；`0xBCC981` 发布相机原点，`0xBCC9A1` 发布角度。
6. 后续调用 renderer 虚表 `+0xB0`、ClientMode 虚表 `+0xD0`，并完成视口、宽高比和 FOV 调整。
7. `0xBC6880` 中的 `0xBC6920` 调用 `0x3A6DB0`，生成包括世界到屏幕矩阵在内的视图矩阵。

ClientModeCSNormal 主虚表为 `0x1B5A560`：

| 字节槽 | 入口 | 作用 |
| --- | --- | --- |
| `+0x78` | `0xCB2930` | 本文称 OverrideView；签名可收敛为 `void(void* clientMode, void* view)` |
| `+0xD0` | `0xCB42C0` | 景深及喷漆预览相关处理；已见代码不写视图原点/角度 |

另核对 CViewRender 虚表 `0x1B39BD0`：`+0xB0` 位于 `0x1B39C80`，指向空函数 `0xBC6260`。这收窄了普通路径在缓存发布后的覆盖风险；不代表已验证所有特殊视图。

### 3.2 视图字段与发布位置

视图块起点为 `renderer + 0x10 + slot * 0x970`。

| 相对 view 的偏移 | 含义 |
| --- | --- |
| `+0x498` | 世界 FOV |
| `+0x49C` | viewmodel FOV |
| `+0x4A0` | 原点，三个 float |
| `+0x4B8` | 角度，三个 float |
| `+0x4D4` | 宽高比；在 OverrideView 之后还会更新 |

| 全局 RVA / 函数 | 含义 |
| --- | --- |
| `0x23D2118 + slot * 12` | 已发布的相机原点 |
| `0x23D2128 + slot * 12` | 已发布的相机角度 |
| `0x23D21F0` | 当前项目用于 ESP 的 slot 0 世界到屏幕矩阵 |
| `0xBC6200(slot)` | 返回上述原点缓存地址 |

**不能在 `0xBCC420` 返回时才移动镜头**：矩阵尚可反映新位置，但模型路径已经可能使用先前发布的相机原点。候选位置是普通本地视图的 `0xCB2930` 原版调用之后、返回调用方之前。

### 3.3 必须让出的相机

`0xCB2930` 的 data-driven 路径通过 `0xC9B3A0(-1)` 获取相机系统，对系统 `+0x28` 的对象执行 `ICamera → CDataDrivenCamera` RTTI 判断，成功时覆盖原点、角度、FOV。另有固定/调试相机分支。

`0xBCC420` 在 OverrideView 前后还有旁观、演示/冻结等分支。实现须正向确认普通本地存活玩家视图，对特殊相机和无法判定的模式让行。仅比较原版前后坐标是否改变不能证明资格成立。具体资格读取器及完整动态时序仍是实施阶段的验证项。

## 4. 玩家本体、手臂和武器

### 4.1 不能从基类外推实际 CS 玩家

| RTTI 类 / 主虚表 RVA | 槽 `+0x360` | 槽 `+0x9C0` |
| --- | --- | --- |
| C_BasePlayerPawn / `0x1B08290` | `0xADAEB0`，本地玩家分支依赖原生第三人称标记 | `0xADAF80`，查询原生第三人称 |
| C_CSPlayerPawnBase / `0x1B43990` | `0xC40D20`，返回 false | `0xC40E40`，返回 false |
| C_CSPlayerPawn / `0x1B468F8` | **`0xC40BF0`**，实际玩家绘制判断 | `0xC40E40`，返回 false |

此外实际玩家 `+0x968` 指向 `0xC35640`，结果为 `!NearEye(pawn)`；`+0x9B8` 指向 `0xC43500`，沿服务和句柄触发相关模型更新，不是一个普通布尔字段 setter。

### 4.2 镜头靠近眼睛的分类

本文将 `0xC4CFC0(pawn)` 描述为 **NearEye**：

- 经 `0xBC6200` 获取当前视图的相机原点缓存。
- 经 `0xC299E0` 获取 Pawn 眼位；经 `0xC297F0` 识别当前本地/观察对象。
- 使用 `cl_view_near_hud_player_eye_dist`（包装对象 `0x23D8628`）或 `cl_view_near_other_player_eye_dist`（`0x23D8638`）。
- 本地路径还包含眼位偏移长度、模型位置等修正，不等同于固定距离阈值。
- `0xC29AF0` 根据 NearEye 和观察模式返回合格的当前第一人称 Pawn。

`0xC40BF0` 先保留队伍、身份状态、呈现场景和有效性检查，再按 NearEye 分流：靠近眼睛时受 `cl_firstperson_legs` 控制；离开眼睛时受 `r_drawcsplayers` 和 `!0xC35650(pawn)` 控制。后者还有靠近骨骼的抑制检查，已见平方距离阈值 144 和 `_iktarget` 排除。

cvar 对应关系由注册函数核实：`0x0E1280` 注册 `cl_firstperson_legs → 0x23D8530`；`0x0E36E0` 注册 `r_drawcsplayers → 0x23D8550`。这意味着镜头离开眼睛并不无条件保证本体被绘制。

### 4.3 当前版本的 HUD 模型类

| 类 / 主虚表 RVA | 绘制槽 | 已确认的依赖 |
| --- | --- | --- |
| C_CS2HudModelArms / `0x1BC1860` | `+0x360 → 0x10511F0` | owner 完整句柄、有效 Pawn、活动武器与对应模型关系、观察模式、`0xC29AF0` / NearEye |
| C_CS2HudModelWeapon / `0x1BC0F18` | `+0x360 → 0x1051370` | 武器 owner、活动武器对应关系，经 `0x104B8D0` 找关联模型，再调用其 `+0x360` |
| 多个世界武器虚表 | `0x7E5120` | owner 与 `0xC29AF0()` 是否相同，以及其他状态检查 |

因此本版本应追踪 C_CS2HudModelArms / C_CS2HudModelWeapon，而不是直接套用旧 C_CSGOViewModel 布局。

`0x0DB0A0` 能确认 `r_drawviewmodel` 的包装对象为 `0x23D1DC0`，但本轮未取得消费者交叉引用，不能据此承诺切这个 cvar 就能管理所有手臂和武器。

**设计含义：** 先在正确时点移动镜头，保留原生模型判断；到墙边镜头缩回眼位时允许自然回到第一人称。是否有模型缓存导致一帧错位、是否必须触发局部重评估，需要实机观察，静态引用不能证明每帧实际执行顺序。

## 5. 碰撞调用 ABI

### 5.1 TraceShape 的六个参数

`0x9E6040` 的接口以 `0xB1FC8D–0xB1FCB7` 原生相机调用点的寄存器、栈参数和被调函数交叉确认：

| 参数 | x64 位置 | 值 |
| --- | --- | --- |
| manager | RCX | 从 `client + 0x20A97C8` **读出的指针值** |
| shape | RDX | 形状描述符地址 |
| start | R8 | 起点 Vec3 地址 |
| end | R9 | **终点** Vec3 地址，不是 delta |
| filter | 第五参数 | CTraceFilter 地址 |
| result | 第六参数 | 结果存储地址 |

函数内部计算 `end - start`。manager 及其首个指针所指的 physics 服务均须有效。函数通过 `0x165F310` 初始化结果；返回值 AL 为：

```text
fraction < 1.0 || result[0xBB] != 0
```

`0x3516A0` 的命中判断与此一致。**返回 false 通常表示无碰撞，不是调用失败；physics 为空时也会返回 false**，所以必须独立验证服务就绪，不能把所有 false 都当成可自由拉远。

### 5.2 形状和结果存储

`0x35F3D0(shape, bounds)` 接受连续六个 float：mins XYZ、maxs XYZ。非退化盒子令 `shape+0x28 = 2`；mins/maxs 完全相同则为类型 0。原生栈中为形状预留 `0x30` 字节。适配时使用清零、对齐的原始存储和已核实 helper，不用网上结构体名猜布局。

结果原生栈跨度为 `0xC0` 字节，与现有 [AimSnapshot.cpp](../src/cs2/sdk/aim/AimSnapshot.cpp) 的 192 字节结果一致：

| 结果偏移 | 已确认用途 |
| --- | --- |
| `+0x08` | 命中实体指针 |
| `+0x78` | 起点 |
| `+0x84` | 终点/命中位置 |
| `+0x90` | 法线 |
| `+0xAC` | fraction；初始化为 1 |
| `+0xBA` | 形状类型 |
| `+0xBB` | 原生 DidHit 使用的标志，初始化 0 |
| `+0xBC` | 相邻标志，初始化 0；语义未完全确认 |

`0x99FE30` 将物理结果 fraction 和标志转换到上述字段。**本轮不将 BB/BC 定名为 startsolid/allsolid**；现有 Aim 代码的参数名不能代替第三人称路径的独立证据。碰撞异常、非法 fraction、这些标志置位时，设计保守退回原版眼位。零长度/起点重叠另有特殊分支，不能仅靠返回 bool 保证完整重叠检测。

### 5.3 原生镜头的 hull 与 filter

基础 hull 常量：

- `0x20BA160`：mins `(-9, -9, -9)`。
- `0x20BA170`：maxs `(9, 9, 9)`。
- X/Y 乘以 `min(max(aspect * 0.5625, 1), aspect * 0.5625)`；Z 不变。指令顺序已核实为 maxss 后 minss。对有限正值，该式等于 `aspect * 0.5625`，**不是最少放大到 1**。

原生 filter 栈跨度约 `0x48`，虚表为 `0x199D338`。通用构造函数：

```text
0x344440(filter, skipEntity, uint64 mask, uint8 layer, uint8 parameter)
```

相机实参对应 mask `0x80`、layer `3`、parameter `15`，随后在 filter `+0x10` 的 64 位字段 OR `0xC0000`。主要布局：

| filter 偏移 | 用途 |
| --- | --- |
| `+0x08` | 64 位 mask |
| `+0x10 / +0x18` | 标志字段；构造时清零，相机另加上述标志 |
| `+0x20` | 排除 Pawn 的完整实体句柄 |
| `+0x28` | 满足原生条件时排除关联 owner 的完整句柄 |
| `+0x24 / +0x2C` | 未使用句柄设为 -1 |
| `+0x30` | `0x9B7400(pawn)` 返回的碰撞相关 short |
| `+0x37 / +0x38` | parameter / layer |
| `+0x39` | `(old & 0xC9) \| 0x49` |
| `+0x40` | 原生本路径为 0 |

`0x9AA540` 生成包含序列信息的完整句柄，不能用 entity index 替代。构造函数根据 Pawn `+0x520` 和虚函数条件处理 owner。推荐使用已核实构造函数加相机专用标志，避免手工复制其实体关系逻辑。

`0xB1FCBC` 读取结果 fraction，`0xB1FCE0` 在 fraction < 1 时将候选镜头距离乘 fraction。该证据证明原生使用体积 sweep 缩短镜头距离，不证明所有 FOV、近平面、动态门和重叠起点均已验证。

## 6. 结论与证据边界

可确定的实现基础：OverrideView 候选入口与普通相机顺序、相机缓存的模型消费者、实际 CS2 HUD 模型类、TraceShape ABI、结果跨度和原生相机 filter/hull。

仍需在实现时完成：

1. 把普通本地视图资格和特殊相机排除条件收敛成经过版本校验的读取器。
2. 确認本体、HUD 手臂、HUD 武器和世界武器在相机缓存更新后的实际重评估时序。
3. 验证碰撞起点重叠、贴墙、旋转、动态物体，以及 FOV 60–140 和宽高比变化。
4. 确认 OverrideView 时读到的 aspect 是否已属于当前视口；`0xBCC420` 后续会更新它，不能直接当成本帧最终投影。
5. 验证卸载与异常边界。此前加载器修复见[崩溃修复记录](cs2-crash-20260923.md)，不能把离线 SEH 通过等同于完整 C++ 异常支持。

本轮仅分析本地二进制和项目代码；未执行这些游戏函数，也未改变正式 DLL。

