# Lunar 中文输入修复设计

日期：2026-09-28。状态：Lunar 1.21.11 首版已实现；原生消息测试通过，实际游戏输入法效果待用户注入验证。

## 当前实现与使用

- ClickGUI → Misc → General → **IME Fix**，默认关闭，仅在 Lunar 1.21.11 注册。
- 原生 `ImeInput` 读取 `ImmGetCompositionStringW(GCS_RESULTSTR)`；`ChatInput` 通过 JNI `NewString` 调用原聊天框 `insertText`，不修改剪贴板。
- 实施时收窄了原计划的跨线程队列：只有窗口所属线程同时通过 Minecraft `isSameThread()` 验证才同步插入，保证提交发生在后续 Enter 处理之前。线程不一致时原样透传；本版不实现异步队列或其他版本适配。
- 保留系统候选窗，未实现候选窗跟随光标的额外定位。不处理只提供 TSF、没有 IMM 结果串的输入法路径。
- 模块关闭/熔断释放 JNI 全局引用；当前聊天框、世界或焦点变化时废弃旧合成。Win32 原始消息在现有窗口入口处理，不经渲染线程消息重放。
- `nova-base.log` 中的 `IME Fix:` 行记录适配器、合成开始和提交长度，不记录聊天内容。若没有效果，这些记录用于区分映射未解析、线程不符、无 IMM 结果串和已经调用文本插入。
- `Client/test/misc/imefix/run.ps1` 使用 C++ 隐藏消息窗口验证 UTF-16、合成归属、读取失败回退、Enter、焦点/目标变化和其他 UI 透传；不等于真实输入法端到端验收。

以下保留分析依据与后续扩展设计；跨线程队列和候选窗定位属于后续能力，当前行为以上述实现说明为准。

## 1. 问题与结论

用户确认：Lunar 1.21.11 中，输入法选字后聊天输入框立即出现 `?`；不注入 Nova 也能复现；正常复制粘贴可以将中文放进聊天框。因此问题不以 Nova 注入为前提，且聊天框能够保存和显示中文。优先排查输入法提交到游戏字符事件之间的路径，服务器发送后的编码不是当前症状的首要解释。

**静态分析尚未证明字符在哪一步变为问号。** 已检查的 Minecraft Unicode 回调、文本过滤、文本框插入以及 Lunar 表情替换没有发现将普通中文转为 ASCII 问号的代码。粘贴正常显著降低了通用字体缺失、文本框不支持中文及无条件后置替换的可能性。下一步比较 Windows 输入法提交、原生字符消息与游戏字符事件的码点，定位首次损坏的边界；不能据此直接认定 GLFW 或某次编码转换有错。

模块建议命名 **IME Fix**，中文说明“修复聊天中文输入”，放在 Misc。首版只有启用开关，默认关闭，仅接管游戏聊天框；不接管 ClickGUI、网页、告示牌或其他输入控件。

## 2. 已核查的材料

- Lunar 主逻辑：`C:/Users/13366/.lunarclient/offline/multiver/lunar.jar`。
- Minecraft mixin：同目录 `modern-0.1.0-SNAPSHOT-all-nomappings.jar`。
- 游戏缓存：同目录 `cache/41523644/2b715c4b/bake.zip`；通过 Recaf 检查已有提取目录 `Client/build-vs2026/test-lunar12111/classes`。本次核查的六个输入相关类与压缩包条目 SHA256 一致。
- 缓存类不是最终运行时类转储，不能据此排除后续 mixin/transformer 修改，也不能仅凭缓存路径确认所有内容均与当前运行实例一致。
- 当前进程使用 Zulu Java 21.0.10，窗口类 `GLFW30`，窗口是 Unicode 窗口。加载的 `glfw.dll` 自报 GLFW 3.4.0，所在 LWJGL 提取目录名为 `3.3.3-snapshot`。
- GLFW 使用 `CreateWindowExW`、`DefWindowProcW` 等宽字符 API，不能解释成“窗口本身只支持 ANSI”。没有查到直接导入的 `Imm*` 函数，也不等于它不支持任何输入法。

### Minecraft 1.21.11 输入路径

| 环节 | 本次反编译标识 | 观察 |
| --- | --- | --- |
| 注册字符回调 | `class_3675.method_15986` | GLFW CharMods callback，传递 int 码点 |
| 键盘初始化 | `class_309.method_1472` | 包装字符事件，提交到 Minecraft 执行线程 |
| 分发字符 | `class_309.method_1457` | 检查窗口和当前 Screen，然后分发 |
| 字符事件 | `class_11905.method_74226` | `Character.toString(int)`，保留 Unicode |
| 可输入字符 | `class_3544.method_57175` | 排除 § 和控制字符；中文允许通过 |
| 聊天输入框 | `class_408.field_2382` | `class_342` 文本框 |
| 字符输入 | `class_342.method_25400` | 校验后调用文本插入 |
| 插入文本 | `class_342.method_1867(String)` | 保留选区替换、长度限制、光标与 responder |
| 读取文本 | `class_342.method_1882()` | 用于比较实际字符串与显示结果 |

长度截断考虑代理项，不应由修复模块硬编码最大长度为 256。粘贴也走文本框插入路径；用户已确认粘贴中文正常，因此可优先复用这个插入终点，让 IME 的完整 UTF-16 提交绕过异常字符入口。仍需确认 IME 结果串本身正确，不能把已经损坏的问号直接送入文本框当成修复。

### Lunar 附加逻辑

KeyboardHandler mixin 在字符回调头部处理 WebOSR 焦点或发送可取消字符事件。已检查代码没有主动将中文替换成 `?`。不同版本安装 CharCallback / CharModsCallback 的分支不能直接当成回调覆盖错误。

ChatScreen mixin 在 `onEdited` 尾部可能重写整个输入值。跟进其表情转换方法后确认：无冒号时直接返回原字符串；有冒号时仅匹配 `:([A-Za-z0-9_+-]+):`，通过 Java Matcher 与 StringBuilder 替换短码，其余文本保持原样。

表情转换类为：
`com.moonsworth.lunar.client.IHIRCRCROHOCOCHOOIORHOHOIOHHRR.OHRIIRCCRIROIORIIHHOCRRIICIROI.OOHHRRCCOIORCHHIIORCHRCRRCHRCI.RCRHRICCICICHOOCCCIRRCIRCICIOC`

方法：`HCICOHCHOHRRIIHRIIIIOHHRROHHHH(String)`。该实现不支持“表情转换把普通中文全部变为问号”的判断。

Nova 的 `OverlayHost.cpp` 给 bind 监听器传字符时存在窄字符接口，但普通透传保留原始窗口消息参数；结合未注入也复现，不能把这一处截断当作当前根因。

## 3. 实施前的最小诊断

在用户自行注入诊断版后，用固定测试文本“中文测试”核对同一次提交的四层数据：

1. 窗口所属线程收到 `WM_IME_COMPOSITION` 时，使用 `ImmGetCompositionStringW(GCS_RESULTSTR)` 获取 UTF-16 结果。
2. 记录对应 `WM_IME_CHAR` / `WM_CHAR` / `WM_UNICHAR` 的码点和顺序，以及合成开始、结束与取消事件。
3. 对比游戏字符入口收到的码点。使用现有 native/JNI 诊断设施；不能为此生成 Java 类或字节码。若现有设施无法安全观测，明确保留该层证据缺口。
4. 比较插入前、插入后及 responder 执行后的文本框实际字符串。若值仍是中文、画面却是问号，转查字体/渲染，不能用输入桥重复插字。

诊断按需启用，只使用测试文本并限制记录量，常规运行不记录聊天内容。

| 观察结果 | 后续处理 |
| --- | --- |
| IME 结果正确，游戏字符错误 | 使用 Unicode 提交桥绕过损坏的字符传递路径 |
| IME 结果正确，插入后被改写 | 定位具体运行时 responder/transformer；桥接本身不足以修复 |
| 文本值正确，仅显示错误 | 单独修复字体/绘制，不启用输入替代 |
| 未收到 IMM 结果或结果已经错误 | 核查实际输入法的 TSF/IMM 路径；不猜测、不把问号反推成中文 |

## 4. 原生修复架构

全部使用 C++ / Win32 / JNI，不生成 Java 源码、辅助类或字节码。

```text
Windows 输入法提交
    → 现有窗口消息入口：读取完整 UTF-16、标记输入归属
    → 有界提交队列：窗口世代 + 焦点世代 + 合成序号
    → Minecraft 客户端线程：验证同一个聊天框仍获得焦点
    → JNI NewString(jchar*, length)
    → 当前 EditBox.insertText(String)
```

### 线程和输入归属

- 在 HWND 所属线程读取 IMM 上下文，成对调用 `ImmGetContext` / `ImmReleaseContext`。先查询字节长度，再按返回长度读取 UTF-16；校验偶数字节、代理对及分配上限。
- 窗口线程不调用游戏 JNI、不访问 Java 对象；游戏线程发布是否可以接管聊天的状态，队列携带世代号。游戏线程执行时再次校验 Screen、具体输入框身份、焦点、启用状态和窗口世代。
- 游戏对象身份由游戏线程维护必要的 JNI 引用并在退出时释放。线程之间不共享 `JNIEnv*`、局部引用或悬空对象指针。
- 在现有客户端输入任务处理位置按序消费，不能仅等待一个任意 tick，使后来的 Enter 提前提交旧文本。文本提交与聊天确认必须保持队列顺序。
- ClickGUI、浏览器和其他 UI 取得焦点时不接管游戏聊天。关闭聊天、失焦、切换世界/窗口后废弃旧队列，防止字符落入新输入框。

### 一次提交只插入一次

- 仅接管已确认有完整结果、目标有效且成功入队的合成提交。
- 在拥有该次合成消息时，阻止原路径再次生成/传递同一提交的字符；具体 `DefSubclassProc`、IME 默认处理与字符消息关系以实际消息追踪验证。
- 不能简单“吞掉后面 N 个字符”或屏蔽所有问号：不同输入法的字符数和结束顺序不同，会误吞下一次正常输入。
- 读取失败、队列满或无法确定消息归属时不启用替代提交，保留原路径并记录原因。已经接管但目标失效的提交丢弃，不重放到新界面。
- 处理输入法选字用的 Enter/Space 和游戏聊天发送之间的冲突：仅在合成拥有该按键期间消费，选择完成后的正常 Enter 必须可发送。
- `WM_IME_ENDCOMPOSITION` 不代表后续不再有字符消息；提交去重状态独立于合成显示状态管理。

### 文本插入和候选窗

- 使用 JNI `NewString` 传 UTF-16，不把 Windows UTF-8 直接交给 `NewStringUTF`，不经过 ANSI/codepage 转换。
- 调用游戏自身插入方法，沿用选区、光标、长度上限和编辑回调，不直接改文本字段、不模拟 Ctrl+V、不改系统剪贴板。
- 沿用系统候选窗，不新增 ImGui 中文输入框。能获取光标坐标时，将 GUI 坐标换算成窗口客户区坐标以调整候选位置；拿不到时保留系统默认位置。
- 首版支持经过实测的 IMM 兼容提交路径。对不提供结果串的 TSF 路径报告未支持；是否新增 TSF 适配取决于诊断，不能承诺任意输入法一律有效。

## 5. 项目接入位置

| 位置 | 责任 |
| --- | --- |
| `src/minecraft/modules/misc/imefix/` | 模块生命周期、开关、聊天目标验证、JNI 文本插入 |
| `src/minecraft/host/input/` | 新增 IME 桥；保存 Win32 合成状态、队列与窗口世代 |
| `src/minecraft/host/render/OverlayHost.cpp` | 在已有窗口消息路径转交原始 IME 消息，兼容直接和排队输入路径 |
| `src/minecraft/compat/mappings/Lunar12111.inc` | 聊天框字段、文本插入、文本读取与焦点能力映射 |
| `src/minecraft/compat/` | 集中解析输入适配信息，不在模块散落混淆名称 |
| `src/minecraft/modules/Registry.*` 与 CMake | 注册模块和原生源文件 |

复用当前 HWND 绑定生命周期，不额外叠加互相竞争的窗口过程。参考 `ui/backend/WindowThreadBinding.h` 的所有者线程安装/卸载方式，但不直接复制其专用属性名。

首批验收 Lunar 1.21.11。其他已支持版本需分别确认聊天对象、方法签名和事件时序；1.7/1.8 的 LWJGL2 路径与现代 GLFW 路径分开适配。映射缺失时禁用该能力并明确说明，不能猜签名调用。

## 6. 生命周期和验收

禁用模块、DEL 熔断和 DLL 卸载时停止接管、清理队列、释放 JNI 引用；在窗口所属线程确认消息处理已停止后才允许卸载。F11、HWND 重建、Alt+Tab 和屏幕切换更新窗口/焦点世代，不复用旧句柄。

验收项目：

- 连续中文选字、中英文混输、标点、支持的补充平面字符；不丢字、不重复、不变问号。
- 光标中间插入、选区替换、退格、粘贴、聊天长度上限，与原版编辑行为一致。
- Enter 选字不提前发送；再次 Enter 正常发送；取消候选不产生文本。
- Lunar Emoji 开关两种状态、命令输入、GUI Scale/DPI 缩放、窗口化和全屏。
- ClickGUI/浏览器输入不进入聊天，关闭聊天或换世界后不补入旧文本。
- 未启用模块时保持原流程；反复开关、熔断和重新注入不残留挂钩。
- 最终以游戏线程读回的 Unicode 字符串和实际显示共同验收，不能仅以 DLL 构建成功判定修复。

本设计不修改发包、服务器聊天校验或系统区域设置。若最终确认是字体显示问题或 Lunar 后置文本重写，应按诊断修订修复入口。

## 7. Lunar 1.8.9 适配（2026-10-01）

模块注册和初始化现支持 Lunar 1.8.9。用 Recaf 核对本机 Lunar bake 的 `Minecraft.isCallingFromMinecraftThread()`、`GuiChat.inputField` 和 `GuiTextField.writeText(String)`；旧版直接使用 MCP 名称，新版继续使用现有名称映射。

旧版目标检查使用 `GuiTextField.isFocused()` 和 `isEnabled`，通过 JNI `NewString` 将 IMM 提交的 UTF-16 文本传给 `writeText`，沿用原版光标、选区、最大长度和 responder。仍要求 HWND 消息线程就是 Minecraft 客户端线程；不会将 JNI 操作迁到任意窗口线程。

新增原生聊天适配器测试覆盖两个版本的绑定、Unicode 插入、焦点/禁用/线程/世界条件、界面切换、异常提交归属和 JNI 引用释放。Windows IME 状态机原有测试继续执行；实际输入法选字和游戏内显示需重新注入后验证。
