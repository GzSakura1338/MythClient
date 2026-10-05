# 资源文件恢复

## 背景
原始 mod 的**全部资源**（贴图/音效/着色器/配置/库元数据）不在 `mio.jar` 里，而是被打包进 `mio-loader.jar`，以 **`mio/<哈希>.bin`** 形式存放（42 个）+ `strings.bin`（字符串表）。

- 加载机制：`nick.MioClassLoader.getResourceAsStream(path)` 先查真实类路径，未命中则算 `Hash.hash(path)`（**MurmurHash2 64位**，种子 `-512093083`，常量 `0xc6a4a7935bd1e995`），用哈希值当 `.bin` 文件名取回内容。
- 即：`.bin` 文件名 = 原始资源路径的哈希。**未加密**，只是路径被哈希掩盖。

## 恢复方法
从代码里提取候选资源路径（`Identifier.of("mio",…)`、字面量、枚举名如 `Mode_9` 的 16 个音效名、着色器名 rainbow/outline/bloom/gradient/solid、post-effect `blur_mask` 等），用相同的 MurmurHash2 算哈希去匹配 42 个 `.bin` 文件名。

**恢复 41/42（97.6%）**，已解包到真实路径：

| 类别 | 数量 | 路径 |
|------|------|------|
| 音效 (OGG) | 16 | `assets/mio/sounds/*.ogg`（kaban/neverlose/css/cod/quake/toolbox/warning/steam/whatsapp/vk/icq/stalker/hover/click/rclick/bodysplat） |
| 贴图 (PNG) | 7 | `assets/mio/textures/*.png`（mio/nav/triangle/warning/shine/skin_protect/overlay） |
| 披风 (PNG) | 4 | `assets/mio/capes/*.png`（mio/nightly/pepsi/tetris） |
| 自定义着色器 | 7 | `assets/mio/shaders/*.frag|.vert`（base.vert + rainbow/outline/bloom/gradient/solid/blur_mask） |
| MC 后处理着色器 | 3 | `assets/minecraft/shaders/post|program/blur_mask.*`（post 定义 + program 定义 + fsh） |
| 配置 | 1 | `assets/mio/sounds.json` |
| 库元数据 | 3 | `META-INF/maven/com.jagrosh/DiscordIPC/pom.xml` + `pom.properties`、`mio-refmap.json`（原始 mixin refmap） |

## 未恢复的 1 个
`mio/-8381037589029380890.bin` —— 一张 **256×256 纯白/透明 PNG**（无代码字面量引用、视觉难辨其用途，疑似 white/mask/glow 基础贴图）。保留为 `.bin`，由 `MioClassLoader` 的哈希兜底机制在运行时按其（未知的）真实路径提供。

## 加载器改动
`nick.MioClassLoader.read()` 原本对缺失 `.bin` 抛异常。因 41 个已解包到真实路径（不再需要 `.bin`），改为**缺失时返回 null**，避免类初始化失败；真实路径资源经委托类加载器直接加载，仅剩的 1 个 `.bin` 仍走哈希兜底。

> 若日后确定那张 PNG 的真实路径，把 `mio/-8381037589029380890.bin` 改名到对应 `assets/…` 路径即可，`mio/` 目录随之可整个删除。
