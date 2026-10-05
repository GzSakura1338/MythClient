# Nova SMTC / MusicHUD

已实现原生 C++ Windows GSMTC 服务与 MusicHUD，注册到所有客户端版本的 `Render → HUD`。系统接入使用 `Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager`，读取播放器发布的会话，不为 Nova 创建播放器，不依赖 Minecraft mapping。

## 使用

启用 MusicHUD 后显示歌曲名、歌手（缺省时显示来源）、封面和播放进度。无媒体时自动收起；打开 ClickGUI 后显示示例卡片，拖动 MusicHUD 标题栏即可调整位置。

| 小齿轮设置 | 行为 |
| --- | --- |
| Source | 默认 Auto，跟随 Windows 当前会话；ClickGUI 设置与 HUD 的 `...` 共用同一个来源选择菜单 |
| Size | 60%–180%，整体缩放 |
| Controls | 默认开启，在 ClickGUI 编辑状态展开播放控制行 |
| Hide When Paused | 默认关闭；开启后暂停时收起，编辑状态仍显示 |

网易云音乐需先在自身的「设置 → 系统」中开启 SMTC，之后会自动出现在来源列表。只开启播放器而未发布 SMTC 时，Nova 无法读取它的媒体信息。已在本机网易云 3.1.40 验证歌曲、歌手和封面读取。

来源菜单使用 ClickGUI 的浅色/深色主题、圆角、内边距、选中标记、Blur 和悬停动画，并在屏幕边缘避让。隐藏右侧滚动条，来源较多时仍可用滚轮滚动。配置继续保存稳定来源标识，界面显示播放器名称。

控制行提供上一首、播放/暂停、下一首、时间和来源菜单。支持 seek 的播放器可拖动进度条，释放后提交一次操作。不可用操作显示为暗色；游戏状态及被 ClickGUI 面板遮挡时不接收播放控制。Controls 关闭后仍可在 Source 设置中指定来源。

卡片背景复用 `ui::hud::Background`，继承 Blur、HUD Background Alpha 和当前 HUD 材质。展开从左侧或右侧进行，控制行高度平滑变化，切歌时文字上下滑动、封面过渡，长标题停留后往返滚动。编辑标题的位移和展开沿用 ClientOverlay。

系统要求 Windows 10 1809 或更新版本；可显示的信息和可用按钮取决于播放器向 Windows 发布的内容。无时长/直播不显示可拖动进度条。歌词和音频频谱不属于 GSMTC 数据，本实现不包含它们。

## 目录与数据流

```text
src/smtc/
  CMakeLists.txt
  SmtcService.h/.cpp          服务入口、后台线程、命令队列与快照
  MediaSnapshot.h            不可变快照、播放进度推算
  MediaCommand.h             有界队列及 seek 合并
  backend/
    Async.h                  可取消的异步等待、事件信号与委托计数
    WindowsSession.h/.cpp    Windows 会话、事件和播放器操作
    ArtworkDecoder.h/.cpp    后台封面读取与 WIC 解码
    NetEaseTimeline.h/.cpp   网易云缺失时间线的只读适配
    TimelineSample.h        原生时间线数据校验与切歌稳定窗口

src/minecraft/modules/render/musichud/
  MusicHUD.cpp               Minecraft 模块注册与公共视图调用

src/ui/hud/musichud/
  MusicHud.h/.cpp            公共设置、动画、编辑态交互与媒体服务调用
  CoverTexture.h/.cpp        封面 GPU 上传、退役和重建
  SourcePicker.h/.cpp        HUD 和两种 ClickGUI 共用的来源菜单
  SourceProperty.h           来源设置类型，兼容原有字符串配置

src/ui/hud/lyrics/
  LyricsOverlay.h/.cpp       独立歌词视图，由公共 MusicHud 管理

test/smtc/                   独立服务、数据模型、OpenGL 与 HUD 测试
```

```text
Windows 媒体事件 → 工作线程 → 不可变快照 → MusicHUD
MusicHUD 编辑态点击 → 有界命令队列 → 工作线程 → Windows 播放器
```

SMTC 不依赖 ImGui、JNI 或 Minecraft；HUD 不调用 WinRT。实现及测试均为 C++，不需要 Java、辅助进程或随 DLL 分发的封面文件。诊断 EXE 仅用于本地测试。

## 服务约定

| 接口 | 行为 |
| --- | --- |
| Start() | 幂等、启动后台线程；上一代线程和事件委托退出后才允许重启 |
| SelectSource(source) | Auto/空字符串跟随系统，否则保持指定来源，来源消失时等待恢复 |
| ReadSnapshot() | 返回只读共享快照，不等待系统调用 |
| Enqueue(command) | 校验会话/曲目标识后入队；执行前再次核对能力和刷新状态 |
| RequestStop() | 禁止新命令、唤醒线程，不在渲染线程等待 |
| WaitStopped(timeout) | 等待工作线程与事件委托排空，回收线程句柄 |
| BeginShutdown() | 永久禁止重启，停止服务；排空后关闭唤醒句柄 |

工作线程使用 `CreateThread` 和 `RoInitialize(RO_INIT_MULTITHREADED)`，通过 Windows SDK ABI/WRL 访问 WinRT。没有 C++/WinRT 协程或 `thread_local`。异步操作查询状态，每次等待最多 3 秒，可取消；不安装异步完成回调。

事件回调只标记更新、唤醒线程。事件订阅以 RAII 撤销，并跟踪委托对象的存活数量。快照约每秒校准，媒体事件立即触发更新，元数据每 5 秒进行后备刷新。来源请求版本和媒体修订号用于丢弃过期结果，防止切换来源时发布旧数据。

命令队列最多 16 条，同一目标相邻的 seek 合并为最后一个。来源变化清空待执行队列，切歌后旧目标命令拒绝执行。操作失败写入快照 error，下一次刷新以播放器实际状态为准。

进度根据系统时间线、LastUpdatedTime、播放速率和 steady_clock 推算；暂停冻结，支持非零起点和上下界。播放器报告不完整时不猜测时长。

### 网易云真实进度

网易云 `3.1.40.205461` 的 SMTC 会话提供元数据、封面和播放状态，但本机实测时间线全部为零。仅当 `cloudmusic.exe` 会话缺失时长时启用 `NetEaseTimeline`：后台只读打开播放器进程，每 250ms 采样原生播放位置和解码层时长。渲染线程不读取进程。该路径设置 `measuredTimeline=true`、`rate=0`，直接展示采样值，不用系统时钟补造进度；SMTC 时间线恢复正常时自动优先使用 SMTC。

旧版 x64 `cloudmusic.dll`（PE 时间戳 `0x6a9e91a6`、映像大小 `0x2711000`）保留经过机器码校验的快速路径。其他构建通过只读扫描代码段，定位 AudioPlayInfo 构造器、播放引擎单例、songId 访问器和进度发布函数，从 RIP 相对指令与虚表引用解析真实地址，不再因时间戳变化直接禁用。

自动定位要求特征唯一、单例初始化与返回地址一致、进度比较与写入地址一致、对象大小和 songId 字段访问方式一致，并核对代码/数据段属性。只支持代码形状和对象布局兼容的更新，不承诺兼容所有未来版本；结构变更或匹配不明确时停止读取，绝不套用旧地址。扫描仅发生在后台发现播放器时，成功后复用布局和进程句柄。

已核对的数据链：

- `audioplayer.onPlayProgress` 发布位置到模块 RVA `0x2193468`，单位秒；相邻字段为缓存进度，不是播放速率。
- 原生播放器位于 RVA `0x2200330`；`+16` 为播放代次，`+80` 为当前 AudioPlayInfo，`+168` 为音频解码后总时长。`+160` 是加载参数，不能作为已加载歌曲时长。
- AudioPlayInfo 的虚表 RVA 为 `0x1d977c0`，`+16` 为 playId，`+112` 为 songId。采样前后核对代次、对象、标识和时长，拒绝跨曲混合数据；连续一致至少 1 秒才发布。
- 只读本用户 `%LOCALAPPDATA%/NetEase/CloudMusic/webdata/file/playingList`，用 songId 找到标题、歌手和目录时长，再与 SMTC 元数据和解码时长核对。仅切歌或失败重试时读取，文件上限 8 MiB，失败重试间隔 5 秒。

网易云更新改变上述代码/结构、无对应播放列表条目、本地曲目/电台、试听时长与完整歌曲不匹配、进程访问失败时，该适配可能不可用，此时 HUD 显示未知时间，不猜测。回退数据只用于显示，不增加拖动 seek 能力；播放/暂停/切歌继续走播放器公开的 SMTC 控制。关闭服务、切换来源和卸载时释放进程句柄，不注入网易云，也不写入其数据。

2026-09-20 实测更新后的构建（时间戳 `0x6aad4b6d`、映像大小 `0x2727000`）自动解析出引擎 `0x22146d0`、进度 `0x21a74d8`、虚表 `0x1da7bd0`。只读探针取得歌曲 `22780387` 的总时长 `234.667` 秒、暂停位置 `229.850` 秒及 46 行歌词。离线回归覆盖代码/数据地址迁移、ASLR、结构变更、匹配歧义、关联地址不一致和读取失败。

只读连续采样诊断（先运行下方 `run.cmd` 构建探针）：

```powershell
Client/build-vs2026/smtc-test/probe.exe cloudmusic.exe --timeline
```

该选项连续输出 10 次快照，未取得原生时间线时返回失败。本次实测位置 `114.840 → 122.800` 秒、总时长 `364.356` 秒。真实暂停、拖动和切歌的游戏内显示仍由用户测试。

## 歌词 HUD

MusicHUD 的 `Lyrics` 开关控制独立歌词 HUD，`Lyrics Size` 控制其缩放。编辑界面可通过 `Lyrics` 标题栏独立拖动，位置保存在 `overlays.musiclyrics`。它仍由 MusicHUD 模块管理，关闭模块一起收起。

歌词正文透明无背景、无 Blur，显示当前句及上下各两句；当前句放大高亮，远处歌词降低亮度，切句整组平滑滚动。可用翻译显示在对应原文下方。长句横向滚动，大跨度 seek 直接定位，普通前后切句平滑移动。编辑态无数据时显示多行预览，游戏中无歌词则收起。编辑标题栏沿用公共 HUD 拖动控件。

目前支持已验证网易云时间线中的 songId，通过网易云自身 HTTPS 歌词接口获取 LRC 和翻译；不按标题猜测歌曲，不携带账号凭据。后台获取，响应最多 1 MiB，网络操作有超时和停止检查，失败每 30 秒重试。同曲缓存内存复用，切歌清空绑定并重新校验，纯音乐无歌词时不显示。关闭 Lyrics 不再发起歌词请求。其他播放器或无法确认 songId 时暂不提供歌词。

解析支持多时间标签、毫秒精度、offset、空行间奏与翻译对齐；根据真实位置查找当前句，暂停不推进，seek 后重新定位。使用 `probe.exe cloudmusic.exe --lyrics` 验证实际获取；已实测取得 46 行歌词，播放推进时当前行从 16 切换到 17。游戏内效果由用户注入测试。

### 当前句播放进度（YRC）

从本机网易云 `web.pack` 的 `app.chunk.d4d4312.js` 和 `183.chunk.d4d4312.js` 核对：客户端调用 `/api/song/lyric/v1` 并请求 `yv=-1`，读取 `yrc`、`ytlrc`，通过 `AudioPlayer.setLyricConfig` 将原始 YRC 交给原生歌词渲染器。YRC 句头 `[起点,时长]`、字词标签 `(起点,时长,标记)` 均以毫秒计，字词起点是歌曲绝对位置，不是句内相对值；前后的 JSON 行是贡献者信息。

Nova 优先解析该 YRC 数据，保存原文的 UTF-8 字节范围和各字词的起点/持续时间，按实际字体宽度裁剪当前句的高亮。字词间停顿保持已唱区域，暂停不自增；前后 seek 直接重新计算。没有 YRC 或格式校验失败时使用 LRC，以本句到下一句（最后一句到曲尾）的时间区间显示句级进度；该回退不代表精确逐字节奏。翻译不冒充有逐字时间数据，继续作为辅行显示，`Lyrics Shadow` 同时控制原文与翻译阴影。

已通过只读探针 `probe.exe 1367158055 --yrc` 实测解析 43 句、495 个字词时间片。模型测试覆盖 UTF-8 范围、半字高亮、字词间停顿、零时长、整句边界、回退定位及异常时间；HUD 独立场景使用带时间片的歌词检验渲染。

## 封面与卸载

封面编码流限制 8 MiB；拒绝边长超过 8192 或总像素超过 32M 的原始图像。WIC 在后台缩放到最长边不超过 256，保持比例，HUD 居中裁剪为方形。同内容封面不重复上传。

通常保留当前和上一张过渡封面，另允许在途 GPU 纹理等待后端释放，总队列上限 8。独立缓存不占用 SkinTextures。ImGui 不自动推进用户纹理的销毁状态，因此 CoverTexture 显式维护 WantDestroy/UnusedFrames，等后端确认 Destroyed 后注销；OpenGL/Vulkan 重建后可重新上传。

Loader 在正常卸载和初始化失败路径调用 BeginShutdown，并等待线程及委托排空。超时保留 DLL 映射并写日志，不强杀线程。同步系统/解码调用可能超过异步等待期限，此时同样保留映射。GlHook 在渲染后端关闭后、ImGui context 销毁前清理封面。

## 验证

从仓库根目录运行：

```powershell
cmd /c Client\test\smtc\run.cmd
cmd /c Client\test\smtc\run_cover_gl.cmd
cmd /c Client\test\smtc\run_hud.cmd
cmd /c Client\test\vulkan\run.cmd
python tools/check-client-layout.py
```

当前通过的检查：

- 真实 Windows 会话与封面读取，取消、委托排空、反复启动/停止以及永久停止门禁；只读取，不操作真实播放器。
- 播放速率、暂停、非零时间起点、边界、未知时长以及命令队列上限/合并。
- OpenGL 封面像素回读、180 次封面替换、有界退役及后端重建。
- Vulkan 封面像素回读、连续替换、窗口重建、120 帧运行、卸载后重建，连同既有 Blur 测试。
- 真实 MusicHUD 与 ImGui 配合模拟媒体服务的独立测试：无会话预览、展开/收起、整体缩放、编辑态按钮、面板遮挡与游戏状态输入隔离。输出预览位于 `build-vs2026/smtc-hud-test/preview.bmp`。

真实游戏中的视觉与真实播放器的播放/切歌/seek 由用户注入测试。构建产物为 `build-vs2026/Release/NovaClientBase.dll`，构建后运行现有静态 TLS 兼容检查。
