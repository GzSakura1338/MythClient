# CS2 HUD 信息

`HudSnapshot` 每 250 ms 从 CS2 分支采集本地信息，再由 Watermark 模块转换为共享 UI 快照。UI 不依赖游戏地址，原有主题、药丸布局、图标和动画继续复用。

- 玩家名：`CCSPlayerController::m_sSanitizedPlayerName`，无值时回退到 `CBasePlayerController::m_iszPlayerName`。
- Ping：`CCSPlayerController::m_iPing`，与计分板字段一致，0 ms 是有效值。
- TPS：最近约两秒接收到的服务器 Tick 增量 / 实际时间，至少采样一秒后显示。不是 FPS，也不固定填写 64。
- Server IP：网络连接的 retry address；可能是直接地址、loopback 或 Steam 中继标识，按游戏提供的字符串显示，不伪造直连 IP。
- 坐标和速度：只采集当前本地存活 Pawn；水平速度使用 Source 的 `u/s`，不是 Minecraft 的 `b/s`。死亡仍可显示玩家名和 Ping。

地址沿用玩家适配器对 Build 14181、client.dll / engine2.dll 镜像大小和 SHA256 的校验。新增 Schema 字段在 `Offsets.h`。2026-09-23 对匹配指纹的 engine2.dll 使用 IDA 核对：

- `engine2 + 0x6A5E0`（`CNetworkGameClient::ProcessTick`）将收到的 `Msg_Tick` 写到网络客户端 `+0x37C`；导出器的 `dwNetworkGameClient_serverTickCount = 0x24C` 与 deltaTick 重合，不用于这里的 TPS 统计。
- `engine2 + 0x6AC10`（`ProcessServerInfo`）读取 `+0x244` 的 server count，并通过 `network + 0x168 → connection + 0x28 → char*` 记录 retry address。

采集使用有上限的 ReadProcessMemory，不依赖反射加载器的 SEH 注册；实体身份、完整句柄、连接状态与 server count 在采集后复查。断线、切图、计数回退、长时间暂停采样都会重置 TPS 历史，失效的数据不继续显示。主菜单/尚未取得信息时共享 HUD 隐藏这些项；Minecraft 保留原有缺省显示行为。

验证：`NovaCs2Smoke --hud-only` 检查信息字段、UTF-8/姓名回退、0 ms、速度单位、死亡/断线/句柄复用、计数回退/切图/TPS。`--probe-hud <PID>` 只读现有进程以验证实际采集，不注入或修改游戏。完整 smoke 生成连接中和主菜单两种 Watermark 图片。
