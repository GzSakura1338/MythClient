# HUD / ClickGUI 局部模糊

在 Panel ClickGUI 的 `Settings → Client` 开启 `Blur`，随后显示：

| 设置 | 范围 | 默认值 |
| --- | --- | --- |
| Blur | 开 / 关 | 关 |
| Blur Strength | 1–6 | 3 |
| Blur FPS | 5–120 | 30 |

这是一项全局 HUD 背景设置，不依赖 ClickGUI 是否打开。ArrayList（开启 Background 时）、TargetHUD、AutoBlockHUD、Watermark、Notifications、NameTags，以及 HUD 编辑标题和 ClickGUI 面板使用局部模糊。文字、图标、头像、进度条仍按原来的顺序清晰绘制。没有面板覆盖的游戏画面不会被改写。

同一 Client 设置页中的 `HUD Background Alpha` 独立于 Blur 开关，范围 0–100%，默认 100%。它统一缩放各 HUD 原有背景颜色和动画的不透明度；100% 保留原来设置，0% 隐藏 HUD 底色及对应模糊层。通知图标的背景和 HUD 编辑标题也受控，文字、头像、进度条以及 ClickGUI 面板本身不受影响。此设置随 cfg 保存。

背景颜色中的 alpha 只控制色调覆盖，不能用它降低模糊纹理的覆盖率；因此即使 HUD 使用半透明底色，也与 ClickGUI 一样完整采样模糊背景。`HUD Background Alpha` 和出现/消失动画则作用于整个玻璃层，按相同比例淡出模糊与底色。TargetHUD 的动画透明度单独传递，避免被误当作背景色透明度。

各个背景直接使用自身的位置、圆角、裁剪及动画进度采样背景纹理。原有颜色作为半透明覆盖层保留；关闭 Blur 时恢复原来的背景颜色和不透明度。

ClickGUI 的小齿轮子设置页、嵌套子页、模块右键菜单、模式下拉、颜色选择器、按键绑定和配置菜单也使用同一份 Blur 缓存及设置。模糊背景位于窗口边框、滚动条和交互控件后方。

主内容卡片与设置卡片分别按实际圆角采样背景，避免多层纯色背景叠加后遮住模糊。仅用于排版的 Settings、Modules、侧栏、顶栏和弹窗内容容器不绘制自己的矩形底色。

`Blur FPS` 只控制背景缓存的更新频率，不限制游戏、HUD 或菜单动画的 FPS。较低数值节省背景处理开销，但快速移动视角时，面板内部的背景更新会更明显地呈现跳帧。修改模糊强度、重新开启开关或改变窗口尺寸会强制刷新。三项设置随现有 cfg 的 `client` 设置保存和加载。

## 实现位置

- `src/ui/backend/blur/Blur.cpp`：读取全局设置。
- `Blur.h`：公共接口和刷新节流。
- `BlurDraw.cpp`：按 HUD 形状绘制模糊背景与颜色覆盖层。
- `OpenGLBlur.cpp`：游戏帧的 GPU 复制、线性缩小 / 放大及纹理缓存。
- `VulkanBlur.h`：VulkanOverlay 的私有实现，管理图像、布局同步和 ImGui 纹理描述符。

算法采用多级线性降采样和升采样，Strength 控制层数。两套后端共用 `FilterPyramid` 采样计划：在低分辨率层额外做两次帐篷滤波来软化采样条带，只重建到半分辨率，再由 ImGui 的线性采样完成最终放大，省去一次全屏纹理回写。场景在 Nova UI 绘制前采集到 GPU 纹理，多个 HUD 共用同一份背景缓存，不逐个 HUD 重做模糊，也不在运行时进行 CPU 像素回读。关闭开关后停止更新并隐藏缓存，保留 GPU 资源以便再次开启；卸载和窗口资源重建时释放。不支持所需 GPU 功能时保留普通 HUD 背景。

新增 HUD 背景时调用 `nova::ui::hud::Background(draws, min, max, color, rounding)`（`ui/overlay/HudBackground.h`），由公共入口应用全局透明度并调用 Blur；HUD 内单独的背景装饰可用 `BackgroundColor` 调整颜色。不要对整个屏幕回写模糊图，也不要将前景文字放进背景采集过程。

有淡入淡出动画的 HUD 使用 `Background(draws, min, max, color, rounding, flags, visibility)`，保留原始背景色，把动画进度传给 `visibility`，不要提前乘入 color 的 alpha。

ClickGUI 弹窗使用 `nova::ui::blur::BeginPopup` / `BeginPopupContextWindow`，成功后仍配对 `ImGui::EndPopup()`。包装接口沿用调用处的 PopupBg、圆角和边框主题，关闭 Blur 或无可用纹理时回退到原生 ImGui 弹窗。OpenGL 测试同时覆盖嵌套弹窗、原生边框和滚动条的绘制顺序，以及控件内部再次拆分绘制通道的情况。

实际卡片使用 `nova::ui::blur::BeginChild` 并配对 `ImGui::EndChild()`；布局容器用原生 `BeginChild` 加 `ImGuiWindowFlags_NoBackground`。卡片背景绘制在父窗口的裁剪范围内，并位于子窗口的原生边框与滚动条下方。

## 验证

在仓库根目录运行：

```powershell
cmd /c Client\test\opengl\run_blur.cmd
cmd /c Client\test\vulkan\run.cmd
```

两者均创建独立的测试窗口，不附加或注入 Minecraft。GPU 像素测试验证局部圆角模糊、面板外画面、清晰前景、强度调节、关闭 / 重开及资源重建。OpenGL 额外验证刷新缓存及 PBO、scissor、sRGB 状态恢复；Vulkan 额外验证放弃提交后的布局恢复、连续帧、卸载及重建。游戏内的最终视觉效果由用户注入测试。
