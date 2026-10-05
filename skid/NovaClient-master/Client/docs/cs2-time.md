# CS2 Time

Render → General → Time 默认关闭。在模块齿轮中选择 Day / Night；Day Brightness 为 1–3（默认 1.25），Night Brightness 为 0.05–1（默认 0.20）。数值是相对地图原生曝光的倍率。绑定、配置和 ArrayList 后缀复用模块系统。

这是本地昼夜亮度模拟：通过游戏原生后处理曝光提亮或压暗场景，不是服务器时间，也不替换天空材质、移动太阳或重算烘焙光照。原地图的太阳、云和阴影仍可能保留白天外观。没有覆盖黑色 UI 蒙层。实际地图观感待游戏内验收。

## 原生入口证据

支持 Build14181；复用 players::Available 的 client / engine SHA256、镜像大小和构建号校验。IDA 对本地 client.dll 的反汇编确认：

- 最终后处理函数 RVA `0x116A1B0` 在 `0x116A5F6` 读取后处理参数 `+0xEC`。
- `0x116A5FE` 加载常数 2，`0x116A606` 通过 IAT `0x1977500` 调用 `tier0.dll!V_powf`，得到 `2^exposureBias`。
- 返回地址 `0x116A60C` 后将结果放入 xmm7，并广播为渲染倍率；下游在 `0x116A971` / `0x116AB9D` 消费该值。
- 安装时额外验证这段 31 字节机器码和 IAT 目标与导出 V_powf 一致。只匹配该返回地址、底数 2 且已连接地图时调整返回值。其他调用原样执行。

UI 只发布一个原子曝光倍率。专用 TimeHook 调用原函数，再调整当前计算结果；不保存实体、视图或材质指针，不写网络字段、曝光控制器和全局 cvar。每次基于原生结果计算，不累计变暗。离开对局或关闭后下一次计算即恢复原生值；卸载停用并排空回调，没有需要回写的持久游戏状态。自有连接查询故障停用模块，原生异常继续向外传播。

## 验证

`NovaCs2Smoke.exe --time-only` 覆盖 Day / Night、亮度边界、非有限值、实际 MinHook 调用路径、其他调用点和底数隔离、主菜单/断线、连续帧不累积、配置往返、关闭恢复、原生异常传播、自有查询故障、阻塞回调排空与重新安装。

离线测试不等同于游戏验证。游戏中需检查不同地图的白天/夜晚亮度、开关切换、换图/断线重连及卸载恢复；静态光照和天空内容不会变成真正动态昼夜。

2026-09-23 验证结果：Release DLL / Smoke 编译通过，无活动静态 TLS；Time 专项、完整 Smoke（含既有 Auto Fire / FOV / recoil 回归）、生产 DLL 反射加载与卸载测试通过。`git diff --check` 通过。仓库布局检查仍被已有 `cs2-third-person-analysis.md` 引用缺失的 `cs2-third-person-design.md` 阻止，与 Time 无关。

交付 DLL：`Client/build-vs2026/Release/NovaClientBase.dll`，22,031,872 字节，构建时间 2026-09-23 03:31:43，SHA256 `35FAC6713229B9AC1E3A054EB465425931B4691A01204515BAF17A4418D9D1F3`。日志位于 build-vs2026 下的 `cs2-time-build.log`、`cs2-time-smoke.log`、`cs2-time-loader.log`。
