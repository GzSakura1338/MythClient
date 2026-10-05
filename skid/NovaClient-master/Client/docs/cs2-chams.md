# CS2 Chams

移植来源：`skid/asphyxia-cs2/cstrike/features/visuals/chams.*`。保留 White / Illuminate 两组可见、遮挡材质定义；Nova 的模块、配置、Hook 和生命周期独立实现。材质文本旁保留原项目 MIT 许可（`src/cs2/sdk/chams/LICENSE.asphyxia.txt`）。构建和运行不依赖 skid 目录。

## 使用

Render → General → Chams，默认关闭。小齿轮提供 Material（White / Illuminate）、Through Walls、Visible Color、Hidden Color，颜色支持透明度；Hidden Color 在 Through Walls 开启时显示。Through Walls 默认关闭。仅处理存活敌方玩家，排除本地玩家、队友和其他实体；独立于 Team 模块。设置、绑定和 ArrayList 使用通用模块系统。

## 当前版本适配

2026-09-24 已迁移到 Build 14183，当前入口、指纹和验证边界见 [Build 14183 mapping](cs2-build14183-mapping.md)。下方为迁移前的 Build 14181 分析记录。

Build 14181 版本对 client、engine2、scenesystem、materialsystem2、tier0、resourcesystem 检查镜像大小和 SHA256。不匹配或材质创建失败时停用 Chams，保留其他可用模块。所有地址为对应模块 RVA：

| 项目 | 已核对的入口或布局 |
| --- | --- |
| scenesystem 绘制 | `0x545D0`，7 个参数，批次步长 `0x70` |
| 绘制记录 | scene `+0x18`，material `+0x20`，颜色 `+0x50` |
| client KV3 构造 | `0x1885AD0`，独立存储 16 字节，参数 1 / 8 |
| tier0 LoadKV3 | `0x125800`，当前导出为 6 参数版本 |
| tier0 KV3 清理 | `0x216730`，mode 0 |
| materialsystem2 创建 | `0x3ADE0`，创建过程复制输入 KV3 数据 |
| resourcesystem | ResourceSystem013 → ResourceHandleUtils001，删除入口 `0x17860` 将零引用资源加入引擎删除队列 |

这些入口及绘制布局已通过本地对应版本反汇编核对。场景 owner handle `+0xB8` 沿用参考项目，尚未在当前游戏运行时独立验证；若其布局已改变，身份检查会拒绝目标。目标和本地玩家均需通过实体句柄序列号、identity 回指及 C_CSPlayerPawn 主虚表校验；读取前后检查连接与实体身份，避免长期缓存实体指针。

## 绘制与卸载

安装时在宿主线程创建四个材质并持有资源引用；每帧只发布设置，在原生场景绘制回调替换选中条目的材质/颜色。遮挡层仅提交连续敌人条目，随后整个批次提交可见层。每次绘制结束和异常展开均恢复原始材质、颜色，避免污染后续物体。最多处理 1024 条的批次，超出时沿用原生绘制。

关闭模块停止替换。卸载先禁用 Hook、等待已进入回调结束，再移除 trampoline 和释放材质引用；资源销毁交由引擎队列处理。创建失败清理已创建资源。自定义读取发生异常时停用适配，原生绘制异常继续向外传播。

## 验证范围

`NovaCs2Smoke.exe --chams-only` 用生产 Hook 和模拟引擎回调验证敌人身份/句柄复用/断线、混合批次、两种材质、颜色、可见/遮挡层、配置往返、关闭、原生异常后的恢复、回调排空、部分创建失败清理和重复安装。

离线测试不加载游戏材质系统，不能验证当前地图上的材质编译、场景 owner 布局或最终 GPU 效果。需要在游戏中检查两种材质、墙后目标、颜色透明度、切图、关闭和卸载；不可将离线通过视为游戏内验收。

2026-09-23 验证结果：Release DLL 和 NovaCs2Smoke 构建通过；Chams 专项、完整 CS2 离线回归及 `run_reflective_unwind.ps1 -ProductionDll` 通过，DLL 无活动静态 TLS。`git diff --check` 通过。项目布局检查仍报告已有断链：`cs2-third-person-analysis.md` 引用了不存在的 `cs2-third-person-design.md`，本次未改动该文档。
