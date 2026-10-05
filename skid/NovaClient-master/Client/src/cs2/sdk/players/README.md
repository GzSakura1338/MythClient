# CS2 玩家快照

`Offsets.h` 当前保存 Build 14184 的地址、字段偏移、实体槽布局及模块指纹。来源为本地 `skid/cs2-dumps/20260925-steam25515854-full`。迁移依据见 [Build 14184 mapping](../../../../docs/cs2-build14184-mapping.md)；下方带日期的内容为历史诊断记录。编译不引用导出文件。

`Players.cpp` 在宿主启动阶段校验 client.dll 和 engine2.dll 的文件 SHA256、PE 镜像大小及引擎构建号，之后允许读取。版本不匹配只使玩家读取不可用，不阻断公共菜单。更新游戏后，重新提取并核对这些数据，再更新集中常量；不要单独放宽版本校验。

`PlayerSnapshot.cpp` 使用可替换的只读 Memory 接口：生产环境由独立 SEH 边界保护 memcpy，测试环境使用模拟内存或 ReadProcessMemory。仅在完整入服状态遍历有限的 controller 槽，通过 m_hPlayerPawn 和完整序号解析 Pawn。采集后再检查实体身份及连接状态，丢弃已删除或更换的对象。当前版本中已验证真实 controller 关联 Pawn 可使用 `c_cs_player_for_precache` 名称，因此同时接受它和 `player`；未关联 controller 的预加载实体不会被枚举。

返回值只含值类型快照：玩家标识、位置、碰撞范围、生命、护甲、队伍和名称。共享 UI 不访问 CS2 内存，CS2 模块负责投影及筛选。

2026-09-23 参考 asphyxia-cs2 的 `features/visuals/overlay.cpp` 优化 ESP：使用当前 Build 14181 Schema 中 `CGameSceneNode::m_nodeToWorld`（0x10）的旋转、平移和缩放转换八个碰撞角点。变换与 Aimbot 共享 `Transform.h`；未取得有效变换时保留原来的 origin + bounds 路径。最大血量缺失/为 0 时按 100 绘制血条，不因此隐藏玩家；有效自定义最大血量仍保留。

投影继续裁剪相机近平面，屏幕边缘保留原始框尺寸并交给绘制裁剪，避免框和血条挤压变形。模块按相机深度从远到近绘制，遮叠时近处玩家信息位于上层。共享 ESP 提供 Compact 布局：对称细框/角框、分离的左侧血条与底部护甲条、顶部名称；仍使用 Nova 字体、主题和 PixelCoverage 抗闪烁绘制。2DESP 的 Outline 默认开启，只控制框的深色描边，文字阴影独立。Minecraft 沿用 Legacy 默认布局。长名称省略并限制在屏幕边距内。

2026-09-22 护甲诊断：对匹配 SHA256 的 client.dll 副本使用 IDA，确认 `m_ArmorValue` 字符串 VA `0x181B410F8`，对应 Schema 描述 VA `0x1820C10F0` 中字段偏移 `0x1CA4`；`m_iPawnArmor` 描述 VA `0x18209E8F0` 的偏移为 `0x924`。控制器读取函数 VA `0x180863310` 反汇编为 `mov eax, [rcx+924h]; ret`。实时只读检查目标的两处护甲均为 0。2DESP 开启 Armor Bar 后保留零护甲的空底条，直接按 0..100 绘制填充，不再换算到整数 0..20 丢失 1..2 点护甲。Minecraft 调用仍使用原来的 0..20 范围及零值隐藏行为。

验证入口是 `NovaCs2Smoke`，包含实体生命周期和近裁剪边界测试；`--probe-esp <PID>` 仅用于人工诊断实时读取，不负责注入。实际渲染由用户注入 DLL 验收。

2026-09-23 本轮 Release 构建、静态 TLS、目录检查和完整 smoke 均通过。新增覆盖最大血量回退、旋转碰撞框、屏幕边缘不压缩、Outline 开关和护甲比例；Compact 布局在 1×/1.333×/2×/2.667× 缩放下的亚像素移动覆盖测试通过。模拟绘制截图位于 `Client/build-vs2026/cs2-test/asphyxia-esp-artifacts/esp.bmp`，日志为 `Client/build-vs2026/cs2-esp-asphyxia-smoke.log`；未注入游戏。
