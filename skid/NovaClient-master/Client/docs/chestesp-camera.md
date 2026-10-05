# ChestESP 相机对齐修复

世界方块 AABB 保持世界坐标；共享投影层使用世界渲染当帧的相机原点、观察矩阵和投影矩阵，避免在 Swap 阶段重新计算相机效果。

- Lunar 1.7.10 / 1.8.9：在原有 ActiveRenderInfo.updateRenderInfo 尾部钩子保存矩阵；原点使用 lastTickPos → pos 插值，时间取实际 updateCameraAndRender 参数。保留 1.7 的 YOffset 补偿和 FreeLook 粒子处理。
- Forge 1.20.1：在 LevelRenderer.renderLevel 入口读取 Camera、PoseStack 和投影矩阵；包含 Forge camera roll、受伤晃动、行走晃动及传送门效果。
- EaseCation 1.21.8 / Lunar 1.21.11：在 LevelRenderer.renderLevel 入口复制 Camera、观察矩阵和世界投影。1.21.11 新增的第三个矩阵是裁剪投影，不用它绘制 ESP。
- 26.3：继续使用现有世界渲染状态中的矩阵与相机原点。

快照仅保存数值，不保存 JNI 对象。帧开始使旧快照失效；错误矩阵不发布；矩阵路径不再叠加手工 bob。所有使用 ReadCameraView 的世界覆盖层共用修复。

验证入口：`Client/test/render/worldcamera/run.ps1`。覆盖 180 帧移动、转头、FOV 变化、晃动、远坐标精度、背后点拒绝、快照复制和失效，以及五种实际客户端 JAR 的相机钩子改写。

构建和离线测试不等同游戏实测。需重新注入验证疾跑、急转头、第三人称、受伤、暂停和切世界时的贴合效果；光影包另行修改渲染管线的情况尚未验证。

本次验证：上述五版本相机钩子和投影测试通过；Lunar 1.21.11 全量检查通过（761 项 JNI、123 项改写检查）；Release DLL 构建及反射加载静态 TLS 检查通过。EaseCation 全量检查仍有与本次修改无关的 FakePlayer 爆炸包 radius 字段不匹配，本次相机钩子的独立检查通过。
