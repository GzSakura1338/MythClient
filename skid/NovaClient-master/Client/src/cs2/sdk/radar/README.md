# 原版 Radar ESP

当前 Build 14184 使用 client.dll RVA `0xEB96E0`，HUD 标志偏移 `0x17808`；已与 Build 14183 的函数指令及新版入口字节核对。版本迁移见 [Build 14184 mapping](../../../../docs/cs2-build14184-mapping.md)。下面的 Build 14181 地址为首次分析记录。

Build 14181 的 `CCSGO_HudRadar` 玩家更新函数位于 client.dll RVA `0xE4EF90`。2026-09-22 使用 IDA 分析匹配现有 SHA256 的二进制：`CCSGO_HudRadar` 名称位于 VA `0x181B93300`，HUD 更新调用链 `0x180E408D0 → 0x180E4EF90`。

玩家更新的可见性条件包含 `!enemy || (hud[96096] & 1) || spotted || observerTarget`。`Radar.h` 集中记录函数地址及 HUD 标志偏移 `0x17760`。`UpdateTarget()` 要求现有玩家 SDK 的构建号、client/engine 哈希检查通过，并额外验证函数入口字节。

`hooks/RadarHook` 在原版玩家雷达更新入口临时设置 bit 0，调用原版更新后，在 `__finally` 中恢复该位；原版对其他位的改动保留。模块跨线程仅发布原子开关，不保留 HUD 或玩家指针，不改写玩家 spotted 数据，不使用 Present 写入游戏状态。关闭模块后下次原版更新恢复正常规则；原版图标本身的淡出时长仍由游戏控制。

Hook 与 DX11 宿主共用 MinHook 的初始化生命周期，卸载先禁用并排空雷达回调，再释放 trampoline；模块版本不匹配只禁用 Radar ESP。独立原生测试覆盖开关、原值恢复、异常恢复、卸载等待与重新安装。

此功能沿用原版地图、玩家标记、颜色和位置计算，不应用 Team 的 2DESP 过滤（用户要求显示双方）。仅能显示游戏已经接收到且原版雷达能识别的玩家实体；服务器未同步的位置、已销毁实体、死亡/离线状态不会被伪造为实时玩家。
