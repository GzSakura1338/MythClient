# Lunar Nick Hider 显示对齐

支持本项目现有全部 Lunar profile：1.7.10、1.8.9、1.21.11。Nova HUD 原先使用 ImGui 绘制真实姓名，没有经过 Lunar Minecraft 字体的 Nick 替换。

新增 `java::PresentationName`，只对显示字符串调用 Lunar 原生昵称替换。Watermark、TargetHUD、Player List、NameTags（Default / Classic）和 CSWIN 已接入；不修改 EntityHandle 的原始名字，也不改账号、UUID、计分板键、队友判定、目标名单或胜利判定。

## 版本入口

- Lunar 1.7.10 / 1.8.9：取 Minecraft.fontRendererObj，通过 JVMTI 按 `$lunar$renderString` 稳定后缀及 `(String)String` 描述符查找字体的私有文字转换入口。避开 per-build Mixin 前缀及 1.7 不同的 Lunar 混淆名。两个本机 bake 的字体入口和 fontRendererObj 字段已由 Recaf 核对。
- Lunar 1.21.11：调用 Lunar util singleton → module manager → Nick Hider → 原生文字替换方法。四个 JNI 描述符以实际 bake 核对；映射集中在 LunarNick.h。
- 其他客户端：返回原字符串。

使用 Lunar 的真实 Nick Hider 规则，所以会遵循 Hide Name / Hide Real Name / Hide Others Names 以及 Apollo 等覆盖状态，不自行猜测一个名字。昵称不被静态缓存，通常跟随 HUD 原有采样周期更新；CSWIN 在绘制时转换胜利者名，播放中修改 Nick 也能更新显示。

UTF-8 / UTF-16 转换保留颜色格式及 Unicode；JNI局部引用限于当前Frame。已有共享JNI访问缓存按原生命周期清理。转换入口不可用或自己的JNI操作失败时回退原文字；不清理进入本函数前已经存在的游戏异常。

## 验证

2026-10-02 更新：通过 Recaf 核对新版 `cc81b79c` bake，新增 Nick Hider
类路径及 Instance / Modules / NickHider / Format 四项映射。新版真实转换入口
为 `CCCOOHCCOHHCIOHICHHOHROIRRHHHO(String)String`；原同名方法在新类中为删除规则，
不能按名称复用。现代版本依次探测两个经过验证的映射组，旧版路径保留。
1.7.10 / 1.8.9 仍使用字体 Mixin 的稳定后缀入口。

`Client/test/render/lunarnick/run.ps1`：三个版本的实际 JAR 入口、1.21.11 JNI映射；原生调用测试 Nick 开关与改变、颜色格式、中文/补充平面字符、非Lunar行为、不可用适配器及异常归属。

`Client/test/render/cswin/run.ps1`：显示Nick后仍接受真实 Winner 名称，拒绝将别人的“Winner: You”当本地玩家胜利；播放时切换Nick更新显示。

Release构建与反射DLL静态TLS检查通过。以上为离线/native测试，真实游戏中Lunar设置切换仍需重新注入后验证。
