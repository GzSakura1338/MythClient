# Panel ClickGUI：Neverlose 源码移植

参考入口是 `skid/neverlose-last/examples/example_win32_directx11/neverlose_menu.cpp`，由参考工程的 `main.cpp` 实际调用。未使用该工程已不再调用的 `gui.cpp` 作为主布局依据。

搜索入口在参考源码中只有放大镜绘制，没有展开框和筛选实现。Nova 将图标位置、尺寸对齐参考，搜索展开/收起采用参考弹窗的 20/s 指数状态及三次缓动，悬停采用 16/s。输入文本保持固定排版，通过裁剪展开；查询由面板统一保存，关闭搜索或点击分类清空查询。搜索结果直接显示可操作的模块开关及小齿轮，操作时保留查询。搜索逻辑是 Nova 的补充实现。

## 代码路径

- `Client/src/ui/clickgui/panel/ReferenceStyle.cpp`：移植的生产绘制代码，包含 Motion、Mix、Text/TextY、Hit、Dots、Chevron、Check、Card、Toggle、RowControl 的选择器/滑条、PopupLayer 的背景和缓动、页面缓动。直接维护 C++ 源文件，各函数注释标明参考位置，不使用生成或同步脚本改写代码。
- `PageLayout.cpp`：按用户要求恢复旧 Panel 的分类模块布局，同时保留目前的视觉。Combat、Movement、Player、Render、Miscellaneous 分类下按 subpage 分组，HUD 统一成组，按模块数量均分左右两列；跨列的分组保留同名标题，避免仅有 General 分组时一侧过长、另一侧空白。两列独立滚动。移除顶部模块选择器和当前模块四卡片配置区。Client 设置使用完整宽度。
- `Elements.cpp` / `PropertyComponent.cpp`：复用 `ModuleToggleRow` 绑定每个模块的开关和小齿轮设置，保留嵌套设置、键位、数值编辑、文本、颜色与来源选择。模块右键仍可设置 Hide in ArrayList。
- `PopupAnimation.cpp`：保留原生弹窗关闭后的短暂绘制快照，让原生 ImGui 弹窗仍可使用参考的退出曲线；快照不处理输入，也不回放含帧局部指针的渲染回调。
- `PanelClickGui.cpp`：主窗口展开及尺寸适配。主窗口 748×576、侧栏 158、工具栏 56、圆角 14。左上角按用户要求恢复原 NOVA 字标：ExtraBold 字体、28 字号、1.2 字距和两秒分色动画，不显示徽标方块及 Minecraft 副标题。

## 保留的参考算法

开关 29×18，状态/悬停分别使用 19/s、20/s 指数动画。滑条宽 80、高 3，滑块半径 5.5，数值框 42×21，进度 14/s。选择器 134×23，弹窗与调用行垂直居中，每项 32。弹窗使用 20/s 指数状态再三次缓动，缩放 .96→1、横移 4；账户使用 17/s 透明度动画。页面使用原 15/s 指数状态再三次缓动、横移 9。主窗口使用 .48 秒四次缓动、缩放 .92→1、上移 16。

## 必要适配与边界

这不是整个参考工程逐字复制：参考工程是 DX11 / ImGui 1.89 的 CS 风格演示，Nova 使用自己的 OpenGL/Vulkan 后端及 ImGui 1.92。绘制尺度经 Scale 适配；固定演示状态数组替换为真实属性；长标签增加裁剪；原生子窗口的裁剪范围跟随动画。配置、搜索、键位、子设置、滚动和浅色主题是 Nova 的功能适配。浅色主题使用中性蓝配色；新配置默认深色，已有配置的选择仍有效。

修改后运行目录检查和生产控件交互测试；参考算法直接对照上述 C++ 函数及原始文件核查：

```powershell
python tools/check-client-layout.py
& Client/test/render/panel/run.ps1
```

## 已验证

2026-09-21：生产 UI 在独立隐藏 OpenGL 窗口中渲染；深色/浅色分别以 60%、100%、150% 运行交互断言。覆盖模块开关、模式选择、滑条拖动/直接输入、子设置、颜色、文本、SMTC 来源、模块选择、配置入口、账户、搜索、所有功能页和 ImGui 上下文重建。验证原生弹窗关闭后的退出动画几何仍存在，动画结束后被清理。

预览位于 `Client/build-vs2026/test-panel/`，不连接游戏、不修改用户配置。配置服务及媒体来源是测试替身，因此上述验证不等于真实配置文件写入或游戏内视觉验收。ESC 关闭主界面沿用 GlHook 现有逻辑。

发布路径为 `Client/build-vs2026/Release/NovaClientBase.dll`，构建后必须通过静态 TLS 检查。由用户自行注入验证。
