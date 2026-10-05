# TargetESP

原生移植来源：`skid/Starain-1.20.1-main/src/main/java/today/starain/module/impl/render/TargetESP.java`。

入口：Render → General → TargetESP，默认关闭。只绘制目标标记，不修改攻击、转头或移动。

## 行为与参数

每次游戏 Pre Tick，优先选择准星指向的其他 LivingEntity；没有该目标时，再读取启用中的 KillAura 当前目标。没有目标立即清除；死亡、离开世界或实体失效时停止显示。不额外加入玩家限定、队友或 AntiBot 筛选，保留参考模块的选取顺序。

| 参数 | 默认值 | 范围 |
| --- | --- | --- |
| Sync Color | 开启 | 跟随 Nova HUD 主题强调色 |
| Color | 白色，Alpha 233/255 | 关闭 Sync Color 时显示调色板 |
| Radius | 0.75 | 0.10–2.00，步长 0.05 |
| Alpha Factor | 1.00 | 0.00–2.00，步长 0.05 |
| Through Walls | 开启 | 关闭时执行分段方块遮挡检测 |

保留参考的 64 段圆环、2 像素轮廓、上下扫描、渐隐裙边、系统毫秒取模与呼吸透明度公式。Alpha Factor 为 0 时仍有 0.35 的基础透明度倍率，这是参考行为。目标位置每帧使用 `RenderPositionPair` 和当前 partial tick 插值，避免只按 20Hz 更新绘制坐标。

## 原生实现与差异

- `src/minecraft/modules/render/targetesp/TargetESP.cpp`：目标采集、世界身份校验、设置、插值、投影及遮挡。
- `RingProjection.h`：三角形/线段在相机近裁剪面上的裁剪；裁剪交点同时插值透明度。
- `src/ui/hud/targetesp/TargetRing.*`：无 Minecraft/JNI 依赖的动画几何和 ImGui 渐变网格，复用当前 OpenGL/Vulkan 渲染后端。
- 不生成 Java 源码或字节码，不增加版本专属 JNI 映射；使用当前客户端现有的实体、相机和射线接口。具体游戏版本的运行效果仍需各版本实测。

参考通过游戏的 BufferBuilder 直接绘制三维图元；本项目采用 ImGui 屏幕投影，不宣称 GPU 管线逐像素一致。近裁剪与视口裁剪避免近距离/出屏时整个光环突然消失或出现巨型三角形。

参考 `Theme.accent(-index)` 读取 Starain 的主题颜色模式；移植读取 Nova 现有 HUD 强调色，不引入 Starain 的整套主题配置。自定义颜色保留独立 Alpha。

参考 `depthTest = !through && player.hasLineOfSight(entity)` 会在目标被遮挡时关闭深度测试，导致关闭 Through Walls 仍可能穿墙。本版修正此开关语义：关闭时，每段裙边中点向当前相机执行方块射线，正常连续跟踪时最多每 50ms 刷新一组 64 段结果，换目标/世界立即刷新。开启时不执行这些射线。

该遮挡是分段近似，不是 GPU 逐像素深度测试；不覆盖其他实体遮挡，墙边部分覆盖、透明方块和快速移动时可能与原世界渲染不同。射线读取失败时隐藏相应段。

## 验证

运行 `Client/test/render/targetesp/run.ps1`：核对动画范围和周期、圆环闭合、近裁剪交点、视口裁剪、渐变 Alpha 网格以及非有限坐标过滤。已通过。测试不替代注入后的视觉验收。

用户自行注入构建产物后，分别验证：悬停玩家/生物、KillAura 当前目标、两种目标同时存在时悬停优先、目标死亡/换世界、Sync Color/自定义 Alpha，以及 Through Walls 开关。
