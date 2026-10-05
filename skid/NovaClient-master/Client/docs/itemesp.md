# ItemESP

Render → Overlay → ItemESP，移植 Raven BS 的 render/ItemESP.java。

- Render iron / Render gold 默认开启；钻石和绿宝石始终显示。
- Max distance 默认 128，范围 32–256，步长 8。
- 忽略存活不足 3 tick、已移除或空的掉落物。
- 沿用 Raven 的位置计算键与物品类型小数后缀，合计每组堆叠数量。
  原算法不是真正的距离聚类，不同位置可能发生键碰撞；此处保留参考行为。
- 每个物品绘制对应资源颜色的立方体和该组数量。
  Alpha 默认开启，小齿轮内 Opacity 可调 0–100%，默认 35%；关闭 Alpha 为完全不透明。
  半径随距离在 0.2–0.4 变化，文字沿用 Raven 的距离缩放。
- 文字使用 Nova Medium12 HUD 字体；Text Shadow 开关默认开启，可关闭文字阴影。

适配：Lunar 1.8.9、Forge 1.20.1、命名版 26.3 的独立 mapping 分支。
Forge 新增 ItemEntity/Items 名称核对 mappings.dev/1.20.1；26.3 新增掉落物
访问按命名 API 接入，未在运行中的 26.3 客户端验证。

与参考渲染接口的差异：Nova 使用现有世界投影 + ImGui 覆盖层，支持现有
渲染后端；按完成的游戏 tick 在 render update 更新纯坐标快照，不跨线程
保存 Java 实体。穿过相机近裁剪面的盒子跳过绘制，与现有 ChestESP 一致。
退出世界、关闭模块和重生清除缓存。

测试：Client/test/render/itemesp/run.ps1。游戏内外观和版本运行验证由用户注入。
