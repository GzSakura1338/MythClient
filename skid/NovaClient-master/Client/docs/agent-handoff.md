# Nova 交接文档

面向接手这个仓库的 agent。读完这份就该能改代码、能构建、能定位崩溃。

## 1. 这是什么

Minecraft 1.20.1 Forge 47.4.23 的外挂客户端。核心是一个**注入 JVM 的原生 C++ DLL**，
不是 Forge mod，不走 mods 目录。

```
Nova/
├── Client/          注入用的原生 DLL（99% 的工作在这里）
├── injector/        注入器：C 的反射式加载 + WinUI 前端
├── skid/            参考源码，不参与构建（见 .gitignore）
├── tools/           辅助脚本
└── build/           注入器的 WinUI 输出目录
```

`Client/` 内部：

```
Client/
├── CMakeLists.txt   两个 target：NovaFatalWidgets（ImGui 静态库）、NovaClientBase（DLL）
├── docs/            文档，就是你现在读的这个
├── src/minecraft/java/api/    对外头文件（base.h、hook_bridge.h、class_edit.h、jni_lookup.h 等）
├── src/             实现
│   └── third_party/ fatalwidgets（ImGui 本地副本）、minhook、jni、vulkan
├── test/bytecode/   临时测试，见 §7
└── build-vs2026/    实际的构建目录
```

## 2. 构建与运行

### 构建 Client

必须**从仓库根目录**跑，路径全写死了：

```bash
"D:/VS2026/VS2026/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" \
  --build Client/build-vs2026 --config Release
```

产物：`Client/build-vs2026/Release/NovaClientBase.dll`。

- **`LNK1104`** = 游戏正在跑而且已经加载了这个 DLL，文件被锁。在游戏里按 `DEL` 卸载后重试。
- 构建目录已经配好，不要重新 `cmake -S`；改了 `CMakeLists.txt` 的话上面那条命令会自己重新 configure。
- 工具链是 VS 2026（`D:/VS2026/VS2026`），不是 VS2022。

### 构建注入器

注入器分两半：WinUI 前端，和它 shell 出去的命令行注入器 `NovaInjector.exe`。

前端 —— `dotnet build` 不行（WinUI 的 workload 走不通），用 VS 2026 自带的 MSBuild：

```bash
"D:/VS2026/VS2026/MSBuild/Current/Bin/MSBuild.exe" injector/NovaInjector.sln \
  /p:Configuration=Release /p:Platform=x64
```

`NovaInjector.exe` 没有工程文件（`cli.c` + `Inject.c` + `LoadLibraryR.c` +
`GetProcAddressR.c`），要手工编，产物直接丢进 `build/`，和 `Injector.WinUI.exe` 并排：

```bash
cmd //c "call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul && cl /nologo /O2 /W3 /DWIN_X64 \
  /Fe:D:/AI/Minecraft/Nova/build/NovaInjector.exe /Fo:<某个存在的目录>/ \
  D:/AI/Minecraft/Nova/injector/cli.c D:/AI/Minecraft/Nova/injector/Inject.c \
  D:/AI/Minecraft/Nova/injector/LoadLibraryR.c D:/AI/Minecraft/Nova/injector/GetProcAddressR.c"
```

`/DWIN_X64` 不能漏：`GetReflectiveLoaderOffset` 用它判断目标是不是 PE32+，漏了就返回 0、
注入直接失败。用法是 `NovaInjector.exe <pid> <绝对 dll 路径>`，失败时退出码非 0。

### 运行环境

| 东西 | 路径 |
|---|---|
| 游戏版本目录 | `C:\Users\13366\AppData\Roaming\.minecraft\versions\1.20.1-Forge_47.4.23` |
| 正常日志 | 同上 `\logs\latest.log` |
| 原生崩溃日志 | 同上 `\hs_err_pid*.log` |
| 客户端配置 | `%USERPROFILE%\.nova\configs\<名字>.json` |
| SRG 反查产物 | 仓库根 `.tmp_mcp/out/`（javap dump，找游戏类先看这里） |

按键：**右 Shift** 开关 ClickGUI，**DEL** 卸载 DLL（卸载时会自动存档）。

## 3. 线程模型 —— 这一节最要紧

DLL 里有两个线程，**它们之间几乎不允许共享可变状态**。违反了会直接杀 JVM。

### RuntimeThread（`src/core/Loader.cpp` 分派到 `src/minecraft/host/MinecraftHost.cpp`）

干这些事，然后就只轮询 DEL：

1. 等 `jvm.dll`，拿 `JavaVM*`，`AttachCurrentThreadAsDaemon`
2. 拿 `jvmtiEnv`
3. `nova::bridge::Install()` —— 装 JNI 桥接（最多重试 60 秒）
4. `nova::hook::EnableTransformer()` —— 开 JVMTI 的 `ClassFileLoadHook`
5. `nova::module::RegisterBuiltinModules()` —— 建模块、`Seal()`、建 `ConfigManager`
6. `nova::glhook::Install()` —— **必须在第 5 步之后**，见下
7. 循环轮询 DEL；按下后 `OpenCircuit()` → `WaitForTeardown()` → `DisableTransformer()` → `UnmapAndExit(self)`

### 渲染线程（游戏主线程）

所有别的东西都在这上面：`ModuleManager`、`ConfigManager`、ClickGUI、输入、EventBus。
它们的入口是 `glfwSwapBuffers` 钩子，也就是 `src/minecraft/host/render/OverlayHost.cpp` 的 `HookSwap`。

### 规则

**除下面这些 atomic 之外，RuntimeThread 不许碰任何 UI / config / module / input / eventbus 的东西。**

| 标志 | 位置 | 含义 |
|---|---|---|
| `circuitOpen` | `src/core/RuntimeServices.cpp`，`OpenCircuit()` / `IsCircuitOpen()` | 该收摊了 |
| `teardownDone` | `src/minecraft/host/render/OverlayHost.cpp` | 渲染线程已经拆干净 |
| `guardTripped` | `src/minecraft/host/render/OverlayHost.cpp` | 异常护栏已经触发过 |
| `sealed_` | `src/minecraft/modules/ModuleManager.h` | 模块列表已冻结，可以遍历 |

两条由此推出的硬性约定：

- **`glhook::Install()` 必须排在 `RegisterBuiltinModules()` 后面。** 反过来的话，渲染线程
  会在模块和 config 还没建好时就开始跑 `Tick()` / `RenderOverlays()`。
- **卸载时的存档在 `TeardownOnRenderThread()` 里做，不在 RuntimeThread 里做。**
  `WaitForTeardown()` 已经保证了顺序，所以存档和绘制天然串行。

这两条是 2026-09-10 那次崩溃修出来的，细节见 §6。

### 反射加载的三个硬约束

DLL 是**内存映射**进 JVM 的（`injector/Inject.c` 的 `LoadRemoteLibraryR`），
不走 `LoadLibrary`，所以 Windows 加载器那套簿记全都没做。三个后果：

- **DLL 里不许出现 `thread_local` / `__declspec(thread)`。** 加载器没给这个镜像分配
  TLS 槽，`_tls_index` 停在 0，所有 TLS 访问会去读**别的模块**的 TLS 块（或空指针），
  从渲染线程或网络线程进钩子就是一次 AV。`src/minecraft/hooks/Hooks.cpp` 里原来那些
  `thread_local` 重入标志已经全改成普通 `static` 了，别再写回去。
- **函数内 `static` 不许带动态初始化。** MSVC 会给这种变量套一层
  `_Init_thread_epoch` 检查，而 `_Init_thread_epoch`（`thread_safe_statics.obj`）
  本身就在 TLS 块里 —— 于是又踩上面那条。读到负数会走慢路径（正确），读到非负数就
  当作「本线程已经初始化过」直接返回缓存指针，也就是**还没构造出来的 NULL 单例**，
  第一次解引用就崩。2026-09-11 把 39 处守卫全拆了：单例和懒加载容器改用
  `src/core/Instance.h` 的 `Instance<T>()`（`std::atomic<T*>` 槽位是**常量初始化**，
  不生成守卫），另外两处是 `TypeIdOf<E>()` 和 `gl::*` 函数指针。注意 `static bool
  busy = false;` 这种**常量初始化**的 `static` 一直是安全的，不用改。
- **卸载要走 `VirtualFree`。** 手动映射的镜像不在加载器链表里，`FreeLibrary` 永远返回
  `FALSE`。`src/core/SelfUnload.cpp` 的 `UnmapAndExit` 先试 `FreeLibrary`，失败再
  `VirtualFree(image, 0, MEM_RELEASE)`，两条路径都能收场；`MinecraftHost.cpp` 直接拿
  `VirtualQuery` 得到的 `AllocationBase` 当镜像基址，不再依赖 `GetModuleHandleEx`。

自查 —— 下面这条应该是 `0`：

```bash
dumpbin //disasm Client/build-vs2026/Release/NovaClientBase.dll \
  | grep -c "gs:\[0000000000000058h\]"
```

**判据是上面这个计数，不是「有没有 TLS 目录」。** 现在的镜像里还留着一个 1 字节、
没有任何读者也没有回调的 TLS 块，`dumpbin //headers` 仍然会打出 `TLS` 那一项，但它是
惰性的：全镜像 `gs:[58h]` 读取为 0，说明没有代码去碰它。Sakura 的 `Meadow.dll` /
`Ribbon.dll` 连 TLS 目录都没有，那是它们源码里恰好一个函数内 `static` 都没有，不能拿
目录本身当验收标准。

## 4. 用户定下的约束（不要违反）

1. **不许往 Minecraft 里注入大量 Java 类。** 每个桥接只生成**一个随机名字的类**，
   逻辑全部写在 C++ 里。参考实现是 `D:\AI\Minecraft\Nova\SakuraTools\native`。
   随机类名的生成在 `src/minecraft/java/RandomName.cpp`，桥接装配在 `src/minecraft/java/HookBridge.cpp`。
2. **从 `skid/` 移植代码时直接照搬。** 用户的原话是「给 forst 的直接滑过来」——
   要的是逐行对齐参考实现，不是重新设计。
3. **代码里不写注释。** 整个 `src/` 和 `include/` 已经被 `tools/strip_comments.py`
   清空了（提交 `f52fd72`，85 个文件、2248 行）。新代码保持一致。
   `CMakeLists.txt` 里的注释是之前就有的，没被清。
4. **临时的测试文件用完删掉**，别留在仓库里。
5. **说中文。**

## 5. 架构速查

### Java 侧（`src/minecraft/java/`）

- `Env.cpp` —— JVM 附着；熔断标志和 `LogTo` 已移至 `src/core/RuntimeServices.cpp`
- `JniLookup.cpp` —— 按名字/描述符找方法 ID
- `RandomName.cpp` —— 随机内部类名
- `ClassFile.cpp` / `ClassEdit.cpp` —— 字节码读写，`src/minecraft/java/api/classfile.h`
- `HookBridge.cpp` —— 生成那个唯一的桥接类并注册 native
- `GameInput.cpp` / `GameOptions.cpp` —— 直接读写游戏的输入和选项对象
- `GamePlayer.cpp` / `GameClick.cpp` —— 玩家状态谓词、准星目标、容器点击

### 点击链路（AutoClicker 用的那条）

服务端只认 `ServerboundInteractPacket`，它由 `Minecraft.startAttack()` 发出。整条链
（2026-09-10 用 javap 逐条核对过）：

```
KeyMapping.click(键)              m_90835_   静态，把匹配该键的 KeyMapping 的 clickCount++
  ↓  clickCount = f_90818_，每个 tick 由下面这个消费
Minecraft.tick()                  m_91398_
  └ if (overlay == null && screen == null)          ← 屏幕开着时整个 handleKeybinds 都不跑
      Minecraft.handleKeybinds()  m_91279_
        ├ 玩家 m_6117_() isUsingItem 为真时：consumeClick() 空转抽干，不攻击
        └ 否则 while (keyAttack.consumeClick()) startAttack()
                                        m_202354_ → gameMode.m_105269_ → 服务端
      Minecraft.continueAttack(boolean)  m_91386_
        └ 参数 = (screen == null && !本次已攻击 && keyAttack.m_90857_()isDown
                  && mouseHandler.m_91600_()isMouseGrabbed)   ← 持续挖方块走这里
```

要点：

- `KeyMapping.set(键, Z)` = `m_90837_` **只写** `f_90817_`(isDown)，不动 clickCount；
  所以「点一下」用 `click()`，「按住」用 `set()`。C++ 侧是 `nova::java::ClickAttack()`
  和 `SetAttackDown(bool)`（`src/minecraft/java/game/GameInput.cpp`）。
- `screen != null` 时 `handleKeybinds()` **根本不执行**，clickCount 会一直积着，
  关屏后一次性爆发。所以任何生成点击的模块都必须自己挡 `ScreenOpen()`。
  Nova 的 ClickGUI 不是 MC 的 Screen，得额外挡 `clickgui::IsOpen()`。
- `continueAttack` 走的是 `isDown` + `isMouseGrabbed`，跟 clickCount 无关。
- 按住鼠标时 `isDown` 常真 = 原版挖方块行为，不要当成 bug。
- **空挥会被 MC 自己罚 10 tick**（`src/minecraft/java/game/GameClick.cpp` 的 `ClearMissTime()` 就是治它）。
  `startAttack()` 的 `case MISS` 分支里有一句
  `if (gameMode.hasMissTime()) this.missTime = 10;`，`Minecraft.tick()` 每 tick
  `if (missTime > 0) --missTime`。也就是说打空之后**接下来 10 tick 里
  `startAttack()` 在第一条 `if (missTime > 0) return false` 就退出，`swing()` 根本
  执行不到** —— 挥手被钉死在 10 tick = 500ms = 2 次/秒，跟点击率完全脱钩。
  `hasMissTime()` = `!GameType.isHardcore()`，**生存和创造都返回 true**
  （1.8.9 是 `!isCreativeMode`，创造模式不罚 —— raven 在创造下手感流畅就靠这个）。
  `LivingEntity.swing()` 自己的去重窗口只有 `getCurrentSwingDuration()/2` = `6/2` = 3
  tick，即 MC 本来能给到 6.67 次/秒，被 missTime 白白砍掉三分之二。
  修法是注入 click 前把 `Minecraft.f_91078_`(missTime) 写 0，**无条件写**。
  别想着「只在打空时清、打实体时保留」——那样会死锁：`attack` 要求 `missTime == 0`，
  而清零 `missTime` 的那条路 `continueAttack(false)` 又只有 `attack` 成功才会走到
  （见下），空气转玩家时会硬吃满 10 tick 的停顿。
- **`startAttack()` 打空时返回 `false`**：`flag` 在 offset 84 初始化为 0，MISS 分支
  `break` 出来从不写它（`swing()` 在 `switch` 之后，照样挥手，但返回值是 false）。
  所以 `handleKeybinds()` 里 `continueAttack(screen == null && !flag && isDown && ...)`
  收到的是 `true` —— 打空那一 tick **不会**清零 missTime。反过来命中实体时 `flag = true`
  → `continueAttack(false)` → 开头一句 `this.missTime = 0`。于是形成死锁：
  `attack` 需要 `missTime == 0`，而清零 `missTime` 又需要一次成功的 `attack`。
- `LivingEntity.swing()` 的去重：`if (!swinging || swingTime >= duration/2 || swingTime < 0)`
  才重置 `swingTime = -1`。字段 `f_20911_`(swinging) / `f_20913_`(swingTime) /
  `f_20912_`(swingingArm)，`m_21011_` 是本体，`m_6674_` 转发到它。
- **`GetFieldID` 的签名必须和字段的*声明*类型逐字一致，写父类不算。**
  `Minecraft.f_91073_` 声明的是 `ClientLevel` 而非 `Level`，写成 `Level` 直接返回
  null，整个 `Resolve()` 作废——`src/minecraft/java/game/GameClick.cpp` 就这么废过一次。
  失败会往 `nova-base.log` 写 `Base: click targets unavailable (...)`，
  括号里哪个是 `0` 就是哪一项没解析出来。

### 事件系统（`src/minecraft/events/`）

对齐 Starain 的设计：

- `EventBus::Subscribe<E>(handler, priority)` → 返回 `Subscription`（RAII，析构自动退订）
- `EventBus::Post<E>(event)` → 可取消事件返回 `IsCancelled()`
- **priority 数值小的先跑**（`Priority::Highest = 0`，`Lowest = 20`）
- `CancellableEvent` 是**独立的**基类，不取消的事件直接从 `Event` 继承
- `Seal()` 之后才允许 `Post`，订阅在分发过程中增删会被延后处理

### 挂钩（`src/minecraft/hooks/`）

- `Edits.cpp` —— 声明表：哪些类的哪个调用点被改写到哪个 native
  （`Redirect` = 改写调用点，`Insertion` = 插入，`Guard` = 方法入口可取消，
  `Append` = 在方法头或尾追加一次调用）
- `Hooks.cpp` —— native 实现，负责把游戏事件转成 `EventBus::Post`
- `Transformer.cpp` —— JVMTI `ClassFileLoadHook`，真正做改写

**当前挂了 10 个类、23 条规则**（16 条改写 + 1 条插入 + 1 条出口拦截 + 5 条追加）。
目标方法的 SRG 名字、字节码指纹、
以及还没挂上的那几十个事件，全在 `docs/event-targets.md` 里，那张表是这个子系统的
主要参考。注意 Forge 运行时会对这些类做二进制补丁，`ClassFileLoadHook` 拿到的是**补丁后**的字节。

### 模块（`src/minecraft/modules/`）

`Module` 基类在 `Module.h`，生命周期钩子是 `onEnable` / `onDisable` / `onUpdate` / `onRender`。
属性通过 `addBool` / `addNumber` / `addMode` / `addColor` 挂，子属性用 `subBool` / `subNumber` / `subMode`。

`Registry.cpp` 的初始化顺序是有讲究的，别改：

```cpp
manager.Add(...) 各个模块
nova::minecraft::InstallUiCatalog();
manager.Seal();
nova::event::EventBus::Get().Seal();
nova::config::SetClientSettings(&ClientSettings::Get().rows());
nova::config::ConfigManager::Get().Init();
```

绑定不走轮询了，见下面的 `src/bind/`；`Module::tick()` 现在不接参数。

注意 `Module::intern()` 把标签存进 `std::deque<std::string>`（`Module.h`）——
`PropDesc::name` 是裸 `const char*`，指向 deque 里的字符串，靠 deque 保证指针稳定。
`addColor` 会把 `ColorProp::descriptors()` 返回的 `PropDesc` 搬进 `settings_`，
那些 `prop` 指针指向 `ColorProp` 自己的成员数组，`ColorProp` 由 `owned_` 持有。

### 快速放方块（`src/minecraft/modules/player/fastplace/FastPlace.cpp`）

2026-09-10 从 `skid/raven-bs/.../player/FastPlace.java` 照搬。要理解它先要理解原版的
右键节流：

```
Minecraft.tick()                m_91398_
  ├ if (rightClickDelay > 0) --rightClickDelay      ← 字段 f_91011_，注意是 int
  ├ LocalPlayer.tick()                              ← Nova 的 PlayerTickEvent 在这里
  └ Minecraft.handleKeybinds()  m_91279_
      └ while (keyUse.consumeClick()) startUseItem()
           ├ if (rightClickDelay != 0) return;      ← 就是这道闸
           └ 放完方块 rightClickDelay = 4           ← 原版冷却
```

所以「加速」= 在递减之后、`handleKeybinds()` 之前把 `f_91011_` 往下压。
raven 是用 Mixin 注入 `runTick` 的「递减后、`profiler.startSection("gui")` 之前」；
Nova 这边对应的是 **`PlayerTickEvent`**（`LocalPlayer.tick` 跑在 `level.tick` 里，
早于同一 tick 末尾的 `handleKeybinds`，次序和 raven 完全一致）。别用 `onUpdate()`——
那是每渲染帧一次，相位相对 tick 随机，`tickDelay=1` 时会比 raven 还快。

- `tickDelay`（0–3，默认 1）：`0` 直接写 0（每 tick 都能放）；否则
  `if (current > delay) 写 delay`。写的是**上限**不是定值，所以只会加速不会减速。
- `activationTime`（0–100ms，默认 0）：按住右键满这么久才生效，防误触。
- `blocksOnly`（默认 true）：主手不是 `BlockItem` 就整个不生效。
- `pitchCheck`（默认 false）：`Entity.getXRot()` < 70° 时不动。
- **数据包那半边**：raven 拦 `C08PacketPlayerBlockPlacement` 且 `direction == 255`
  （对空气用物品），命中就 70% 概率取消，用来压服务端的 use-item 频率。
  1.20.1 的 `ServerboundUseItemPacket` **只有 `InteractionHand` + `sequence` 两个字段，
  没有 ItemStack 也没有方向** —— 它自己就是「direction == 255」那一类（对块用的那个是
  `ServerboundUseItemOnPacket`，另有其人）。所以 raven 的
  `packet.getStack() instanceof ItemBlock` 检查改成读**玩家主手**。
  这也是 `nova::java::IsUseItemOnlyPacket()` 和 `IsUseItemPacket()` 必须分开的原因 ——
  后者把 `useItemOn` 也算进去了，用在 FastPlace 里会把挖方块的包一起吞掉。
- JNI 层在 `src/minecraft/java/game/GamePlace.cpp` / `src/minecraft/java/api/game_place.h`：
  `RightClickDelay()` / `SetRightClickDelay()` / `UseKeyDown()`（读
  `Minecraft.f_91066_`→`Options.f_92095_`→`KeyMapping.m_90857_`）/ `HoldingBlockItem()`
  （主手 stack → `getItem()` → `IsInstanceOf(BlockItem)`）/ `PlayerPitch()`。
- raven 的 `mc.inGameHasFocus` 对应 `MouseGrabbed() || ScreenOpen()` 两个取反。
- **没搬过来的**：`ignoredHeldItems`（ItemListSetting）和 `blockBlacklist`
  （BlockListSetting）以及它们的开关。Nova 的 `Properties.h` 只有
  Number / Bool / Mode / Color 四种属性，没有列表类型。这两个开关在 raven 里默认
  就是关的，所以**默认配置下行为和 raven 一致**。

### 按键绑定（`src/bind/`）

照着 `skid/Frost2-master` 的输入层搬的（`InputListener` / `InputManager` /
`IBindable` / `BindableManager` / `OverrideOnKey` / `OverridePopupComponent` /
`InputUtils.getKeyName`）。四层，优先级越大越先看到事件，返回 `false` 表示吃掉：

| 类 | 优先级 | 干什么 |
| --- | --- | --- |
| `BindCapture` | 95 | 正在录绑定时吃掉所有输入，第一个按键就是新绑定 |
| `BindableManager` | 80 | `IBindable` 注册表；键盘 `processInput(key, state)`，鼠标 `processInput(-1 - button, state)` |
| `InputListener` 默认 | 10 | 没人特化的回调默认放行 |

`GlHook.cpp` 的 `DispatchToBind()` 在**每个** `WM_KEYDOWN` / `WM_*BUTTON*` /
滚轮 / `WM_CHAR` 上喂 `InputManager`，位置在 ImGui 和游戏之前（自动重复的
`WM_KEYDOWN` 按 `lp` 的 bit30 丢掉，否则按住一个绑定会疯狂翻转）。
`HandleWndProc` 里 `taken` 只掐游戏那一路，ImGui 照样收得到事件——不然录绑定
时吞掉 mouse-up 会让 ImGui 一直以为左键按着。

**键盘消息里 Shift / Ctrl / Alt 只有中性码**：按右 Shift 收到的是
`wp=VK_SHIFT(16)`，不是 `VK_RSHIFT(161)`，左右只体现在 `lParam` 的扫描码里
（右 Shift 是 `0x36`）。`KeyOfMessage()` 负责还原成 `VK_RSHIFT` / `VK_RCONTROL`
/ `VK_RMENU`，做法和 GLFW 的 `translateKey` 一样（Shift 用
`MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX)`，Ctrl/Alt 用 bit24 的扩展位）。
绑定表里存的、UI 上显示的，都是还原之后的码。

`BindProperty` 既是属性又是 `IBindable`（keycode + requireHold），构造时自己
注册进 `BindableManager`，析构时注销。`Module` 直接持有它，把它作为 `Bind` 行
放进 `settings()`，UI 侧由 `Elements.cpp` 的 `BindElement` 画（右边那个键名按钮
右键 = 清空）。**模块不再自己轮询按键**：`Module::tick()` 只做 enabled 边沿，
`setEnabled` 由 `BindProperty` 的 `setStateChangeConsumer` 回调驱动；菜单开着时
响不响应由 `setGate()` 决定，`Module` 把 `keepBindWhileMenuOpen()` 接在那里
（`ClickGuiModule` 的 RSHIFT 就是靠这个）。

**门控必须自己挡 MC 的界面。** 绑定走的是 `WM_KEYDOWN`，**完全不经过 Minecraft**，
所以聊天栏开着时打字，只要那个字母正好是某个模块的绑定键就会触发。`Module` 的
gate（`src/minecraft/modules/Module.cpp`）因此是两段的：

```cpp
keybind_.setGate([this] {
    if (nova::java::ScreenOpen())      // 聊天栏 / 背包 / 暂停菜单…一律不响应
        return false;
    return !nova::clickgui::IsOpen() || bindWhileMenuOpen_;
});
```

`ScreenOpen()` 只看 `Minecraft.f_91080_` 是否为 null，而 `clickgui` 的 `open` 是
纯 atomic、从不碰 `screen`，两者不冲突。gate 只在**按键真的匹配上某个绑定**时才求值
（`BindableManager::ProcessInput` 先比 keycode），所以这次 JNI 调用的频率等于
「按到绑定键」的频率，可以忽略。WndProc 由 `glfwPollEvents()` 在渲染线程派发，
和 `AutoClicker::onUpdate()` 同线程，JNI 是安全的。

一个已知的边界：`WM_KEYDOWN` 是先经过我们的 WndProc、再 `CallWindowProcW` 给 GLFW
派发给 `KeyboardHandler`。所以**按下去开聊天栏的那一下，gate 看到的还是「没界面」**。
真要抠死，得连「这个键会不会开界面」一起判断，暂时没做。

按键码约定：`0` = 没绑；正数 = Windows 虚拟键码；鼠标 = `-1 - button`
（LMB = -1，RMB = -2，…）。显示名走 `bind::KeyName()`。

### 配置（`src/config/`）

- `%USERPROFILE%\.nova\configs\` 下每个 `<名字>.json` 一个配置
- `Config` = 一个文件；`ConfigManager` = 目录 + 当前名字 + 状态行
- 名字过滤在 `SanitizeConfigName()`：只留字母数字、空格、`-`、`_`、`.`，去掉首尾空格和点，剥掉 `.json`
- 序列化是 `src/utils/Json.h/.cpp`（自己写的，值语义），**不是** nlohmann
- 重名属性靠 `NameKeys` 加 `#2` 后缀区分

### UI（`src/ui/`）

两套菜单，`nova::clickgui::Mode` 切换：

- **Panel**（默认）—— `ui/clickgui/panel/` 下自己写的面板系统。
  `PanelClickGui` → `TopBar`（标题、配置下拉、搜索）+ `CategoryPanel`（左侧分类列 + 右侧卡片）。
  `MainPanel` 派生 `ModuleListPanel` / `ClientSettingsPanel`；属性 UI 由
  `PropertyPanel` / `PropertyComponent` / `Elements.cpp` 里的 `SwitchElement`
  `SliderElement` `SelectorElement` `ColorSelectorElement` 构成。
  主题在 `Theme.cpp`，字体在 `Fonts.cpp`（4 个内嵌 base85 字体，占了大半个 DLL 体积）。
- **Classic** —— `ui/clickgui/Menu.cpp`，FatalWidgets 的原版菜单。

`GlHook.cpp` 是两者的宿主：建 ImGui context、接 Win32 后端、每帧调
`RenderOverlays()` 和 `clickgui::Render()`。窗口过程 `HookWndProc` 负责把输入喂给
ImGui、并在菜单打开时吞掉游戏输入。

**当前隐藏行为（2026-09-18）**：选项改为「Hide in ArrayList」，由
`Module::hiddenInArrayListProperty()` 管理。通过右键模块行打开独立菜单设置，
不放在齿轮的模块参数中；Panel 和 Classic 都支持。模块保持在 ClickGUI 中可见；ArrayList
用该状态控制现有行的退出/进入动画。配置写入 `hiddenInArrayList`，读取旧配置时兼容
`hiddenInClickGui`，新键优先。「Show hidden modules」及 ClickGUI 隐藏过滤已移除。
以下 2026-09-10 的描述保留为历史故障记录，其中隐藏 ClickGUI 的行为已被替换。

**单个模块在 ClickGUI 里隐藏**（2026-09-10，历史行为）：**右键模块卡片的任意位置**（标题或下面
的参数区都行）→ 弹出菜单里一个标准开关行「Hide in ClickGUI」。开关状态在
`Module::hiddenInClickGuiProperty()`，是个真的 `Properties::BoolProperty`——不是裸
bool，因为菜单里那行要交给 `PropertyComponent` + `MakeElement` 去画，
才能和其它地方的开关长得一模一样。它**不在 `module.settings()` 里**，所以卡片内不会
多出一行；配置里走 `hiddenInClickGui` 这个单独的键（`config/Config.cpp` 的
`BuildModule` / `ApplyModule` 各一行）。找回的办法在 Client Settings →
「Show hidden modules」，打开后被隐藏的模块会照常显示、标题染成 `iconHighlightColor`。

- `ModuleListPanel` 每帧 `Collect()` 一次可见模块列表，和上一帧的指针列表比对，
  变了才 `Rebuild()`——因为右键隐藏这个动作必须当帧生效，光靠搜索串变化来判断不够。
- 右键判定用的是 `ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)` +
  `IsMouseClicked(ImGuiMouseButton_Right)`，**不是** `IsItemClicked`。因为参数区是
  嵌套子窗口（`panel` → `props`），卡片的标题栏 `InvisibleButton` 盖不到那里；
  `ChildWindows` 这个 flag 会把 `ParentWindow` 链往上走到当前窗口，正好覆盖整张卡片。
  用 `IsItemClicked` 那种写法只能命中标题那一行。
- **`OpenPopup` 和 `BeginPopup` 必须在同一个 window 作用域里**。ImGui 的 ID 栈是
  **每个 window 一份**（`ImGuiWindow::IDStack`），`BeginChild` 会换 window，
  在子窗口里 open、到外面 begin 就是两个不同的 ID，popup 永远不显示。
- **popup 的宿主被析构时要自己收尾。** `PropertyComponent` 里的开关按下后不会关
  popup（只有 `MenuItem`/`Selectable` 会），而模块一旦被隐藏、`showHiddenModules`
  又没开，下一帧 `Rebuild()` 就把这个 `PropertyPanel` 析构了，popup 会变成没人
  `BeginPopup` 的孤儿——它会吃掉后续点击。所以渲染完那行后判断
  `hidden && !showHiddenModules` 就 `CloseCurrentPopup()`；
  开了 show-hidden 时卡片还在，popup 就让它留着，方便来回切。
- **`SetNextWindowSizeConstraints` 里不打算约束的那一轴必须写 `-1.0f`，不能写 `0.0f`。**
  `imgui.cpp` 的 `CalcWindowSizeAfterConstraint()` 判断的是
  `(cr.Min >= 0 && cr.Max >= 0) ? ImClamp(...) : SizeFull`——**只有负数才代表「不约束」**。
  min/max 都填 `0.0f` 会走 `ImClamp(y, 0, 0)`，高度直接夹成 0，再由 `CalcWindowMinSize`
  兜底成十几个像素。现象是右键后只弹出一条 210×11 的空细条，里面的行全被裁掉，
  一个字都看不见。这里踩过。
- **`PropertyComponent` 按值持有 `PropDesc`（不是引用）。** 原来存的是
  `const PropDesc&`，`PropertyPanel` 里给 `hiddenRow_` 传的是个临时对象
  `PropDesc("Hide in ClickGUI", ...)`，构造完就悬空了；子行 `children_` 同理。
  改成按值持有后，临时对象也能安全传进来。`PropDesc` 内部只有
  `const char*` / `AbstractProperty*` / `vector` / `std::function`，拷贝代价可以忽略，
  而且 `name` 指向的是字符串字面量或模块里 `intern()` 过的串，生命周期本来就比
  面板长。往里加新的行时不用再操心谁活得久。
- **空分类陷阱**：第一版上线就踩了 —— `Category::Player` 当时只有 FastPlace 一个模块，
  右键一藏，整个分类空掉、直接显示 `Nothing matches`，用户完全不知道发生了什么，
  也不知道怎么找回来。现在 `ModuleListPanel` 会数一下「被搜索匹配但被隐藏」的条数
  （`hiddenCount_`），空列表时区分两种情况：只有隐藏导致的空 → 显示
  "Everything here is hidden from the ClickGUI" 并给一个
  **Show hidden modules** 按钮（直接写 `ClientSettings::setShowHiddenModules(true)`）；
  搜索过滤导致的空 → 维持 `Nothing matches`。
  **以后往某个分类里加模块要有心理准备：任何分类只要被藏空，用户就只剩这一个出口。**

## 6. 2026-09-10 的崩溃与修复

### 现象

游戏进程直接死，`latest.log` 干净地停在某一刻、没有 Java 异常。
`hs_err_pid45768.log` 里：

```
Current thread: JavaThread "Render thread" [_thread_in_native]
C [KERNELBASE.dll+0xc41ca]
C [VCRUNTIME140.dll+0x52c7]
C [NovaClientBase.dll+0xa1c8f]
JNI.invokePV → GLFW.glfwSwapBuffers → RenderSystem.flipFrame → ...
siginfo: EXCEPTION_UNCAUGHT_CXX_EXCEPTION (0xe06d7363)
```

### 根因

`std::bad_alloc` 从 `std::string::assign` 抛出，位置在 swap hook 里，
说明读到了一个 `_Mysize` 被破坏的 `std::string`。反汇编定位到：

```
std::string::assign(const char*, size_t)  @ RVA 0x11230
  → operator new @ RVA 0xa0eb0
    → __scrt_throw_std_bad_alloc @ RVA 0xa1c70
      → _CxxThrowException → RaiseException
```

`_Count ≥ 0xFFF`（走了 `_Big_allocation_threshold` 那条路），量级上只可能是垃圾长度。

破坏源是**跨线程竞争**：`Loader.cpp` 的卸载流程里

```cpp
OpenCircuit();
configs.SaveCurrent();   // ← RuntimeThread
```

`OpenCircuit()` 只是置个 atomic，渲染线程要**下一帧**才会拆；但 `SaveCurrent()`
是立刻执行的。于是它和渲染线程正在进行的 ClickGUI 绘制并发跑，同时读写
`ConfigManager::current_` / `status_` / `configs_` —— 渲染线程读到的 `_Mysize`
和 `_Bx` 指针来自两个不同的对象状态，于是 `operator new(天文数字)` 失败。

按 DEL 卸载 DLL 好重新编译，每次都正好撞进这个窗口，所以看起来像偶发。

同一个毛病还有一处：`glhook::Install()` 原本排在 `RegisterBuiltinModules()` **前面**，
渲染线程在模块表还没建好时就已经在跑了。

补充：整个 ClickGUI / config 路径的源码逐行读过，没有悬空引用、越界或生命周期错误，
问题全部在调用时机上。

### 改了什么

| 文件 | 改动 |
|---|---|
| `src/minecraft/host/render/OverlayHost.cpp` | `TeardownOnRenderThread()` 里加卸载存档（带 `try`/`catch`，不能让它抛） |
| `src/minecraft/host/render/OverlayHost.cpp` | `HookSwap` 拆成 `RunFrame()` + 带护栏的 `HookSwap` |
| `src/minecraft/host/render/OverlayHost.cpp` | 窗口过程拆成 `HandleWndProc()` + 带护栏的 `HookWndProc()`，新增 `PassToGame()` |
| `src/core/Loader.cpp` | 删掉 RuntimeThread 里的 `SaveCurrent()` 和对应的 include |
| `src/core/Loader.cpp` | `glhook::Install()` 挪到 `RegisterBuiltinModules()` 之后 |
| `CMakeLists.txt` | 删掉临时加的 `LINK_FLAGS "/MAP"` |

**异常护栏**是独立于根因的必要修复：之前 `HookSwap` 和 `HookWndProc` 里没有任何
`try`/`catch`，任何 C++ 异常都会顺着 JNI 逃出去、直接杀死 JVM。现在捕获 → 打日志
（含 `what()`）→ `OpenCircuit()`，overlay 自己关掉、游戏继续跑。

改完之后共享可变状态只在渲染线程上访问，RuntimeThread 只剩下 atomic。

### 同日稍后：切换 ClickGUI Mode 崩溃

**现象**：在 Panel 里把 ClickGUI 模块的 `Mode` 从 `Panel` 改成 `Classic`，当帧即崩。
`hs_err_pid49784.log`：

```
EXCEPTION_ACCESS_VIOLATION (0xc0000005) ...
siginfo: reading address 0x0000000000000000
RAX=0 RBX=0 RCX=0
JNI.invokePV → GLFW.glfwSwapBuffers → ...
```

`RCX` 在 x64 下就是 `this`。寄存器全零 + 读地址 `0x0`，即通过空指针做虚调用。
栈上还散着 `0xbf800000`（-1.0f）和 `0xFF666666` / `0xFFFAFAFA` 这类常量：前者只
可能来自 `FeatureTree.cpp` 里 `FeatureGroup{..., -1.0f, ...}` 的 `height`，后者是
`Menu.cpp` 里的色值——崩溃点就在 `Fatal::Menu::draw()` 的 Groups 遍历循环里。

**根因**：`Menu.cpp` 的 `getPropElement()` 只认三种属性：

```cpp
if (dynamic_cast<Properties::BoolProperty*>(prop))  return &CheckBoxElementImpl;
if (dynamic_cast<Properties::NumberProp*>(prop))    return &SliderElementImpl;
if (dynamic_cast<Properties::ModeProperty*>(prop))  return &ComboBoxElementImpl;
return nullptr;
```

但 `Module.cpp` 给**每个**模块的第二行设置就是 `Bind`：

```cpp
settings_.emplace_back(intern("Enabled"), &enabledSetting_);
settings_.emplace_back(intern("Bind"), &keybind_);   // BindProperty
```

`BindProperty` 三者都不匹配 → 返回 `nullptr`，紧接着那行
`const auto eSize = element->getSize();` 直接虚调用。`addColor()` 展开出来的
`ColorProperty` 行（Static Color / Gradient Color 1-6）同理。

Classic 菜单只在 Mode 切过去之后才第一次绘制，所以这条必崩的路径一直没被触发。
类别 0（COMBAT）有模块，第一帧就会走到。

**改了什么**：`src/ui/clickgui/Menu.cpp` 两处（主属性行、SubProps 弹窗里的行）拿到
`element` 后判空，为空则整行跳过。副作用是 Classic 模式下 `Bind` 和颜色行不再显示；
Panel 模式不受影响（那边 `Elements.cpp` 有对应的 `BindElement` / `ColorSelectorElement`）。

## 7. 已知问题 / 待办

**没验证过的**：

- 那次崩溃修复之后还没在游戏里跑过，需要重新注入确认。
- `AutoClicker`（`src/minecraft/modules/combat/autoclicker/AutoClicker.cpp`，照 `skid/raven-bs` 的
  `AutoClicker.java` 逐行搬的）：SRG 名字全部核对过，但**还没在游戏里打过人**。
  要确认的是 JNI 句柄解析没失败——失败会往 `%APPDATA%\.minecraft\nova-base.log`
  写 `Base: click targets unavailable (...)` 或 `Base: attack click unavailable (...)`，
  括号里哪个是 `0` 就是哪一项没解析出来。
- `AutoClicker` 已经修过三轮，**都还没进游戏验证**。
  第一轮：`GameClick.cpp` 的 `f_91073_` 签名由 `Level` 改成 `ClientLevel`（原来恒
  解析失败，导致 `ScreenOpen()` / `InventoryScreenOpen()` 恒 false，背包点击完全没
  用、开背包时点击还会堆积到关背包后爆发）。
  第二轮（治「对着空气挥手时快时慢」）：`src/minecraft/modules/combat/autoclicker/AutoClicker.cpp` 的点击
  主循环三处调整——
  1. **去掉**了第一轮加的 20 TPS 门控（`tick = now / 50`）。它看着像在「对齐
     raven」，实则有害：`onUpdate()` 跑在渲染帧上，只要有一帧耗时超过 50ms，
     `GetTickCount64() / 50` 就会**跳过**一个 tick 号，那一 tick 的点击没注入，
     而 `nextClickTime_` 也没推进，下一 tick 一次性补上 2 个 —— 但 `swing()` 对同
     tick 的重复调用去重，**漏掉的那次挥手永久丢失**。
  2. `GetTickCount64()` → `std::chrono::steady_clock`（`NowMillis()`）。前者精度
     只有约 15.6ms（系统时钟节拍），对 33–180ms 的间隔来说量化误差太大。
  3. 每帧最多注入 1 个 click（`if (clicks > 1) clicks = 1;`）。`handleKeybinds()`
     每 tick 用 `while (consumeClick())` 一次性消费**所有**累积 click，而
     `LivingEntity.swing()` 会挡掉同 tick 的重复调用 —— 所以一个 tick 里塞几个
     click 都只算一次挥手，多余的既不出效果、又白白消耗掉那段时间片。
  第三轮（治「对着空气挥手还是一下一下的、不连贯」）：前两轮都没治到点子上，
  真凶是 MC 自己的空挥惩罚（见 §5「点击链路」里 missTime 那条）。改了两处：
  1. `src/minecraft/java/game/GameClick.cpp` 新增 `AttackTargetIsEntity()` 和 `ClearMissTime()`，
     后者把 `Minecraft.f_91078_`(missTime) 写 0；顺手把 `BlockTarget()` 里查
     `hitResult.getType()` 的那段抽成 `TargetTypeIs(env, client, expected)` 供两者共用。
  2. `AutoClicker::onUpdate()` 注入 click 前无条件 `ClearMissTime()`。
     （中间试过加 `if (!AttackTargetIsEntity())` 的实体判断，结果「由空气转到玩家会停顿
     一下」——就是上面说的死锁，已删掉，`AttackTargetIsEntity()` 那套也一并删了。
     验证过 `LocalPlayer.m_6674_` 每次挥手都发 `ServerboundSwingPacket`，6.67 次/秒
     跟正常挖方块/连打的包频一样，不需要额外节流。）
- ClickGUI 切到 `Classic` 的崩溃已按上面的方式修掉，但还没在游戏里切过 Mode 验证。

**功能缺口**：

- **没有列表属性类型。** `config/settings/Properties.h` 只有 `NumberProp` / `BoolProperty` /
  `ModeProperty` / `ColorProperty` / `ColorProp`。从 raven 搬的任何用到
  `ItemListSetting` / `BlockListSetting` 的模块（FastPlace 的 `ignoredHeldItems`、
  `blockBlacklist`）都缺这一块。补齐要动四处：`Properties.h` 加 `AbstractProperty`
  子类、`Module.h` 加 `addList()`、`ui/clickgui/` 的 `PropConfigurerElement` 加渲染元素、
  `config/ConfigManager` 加 JSON 序列化——另外还缺一个「从物品/方块注册表里挑条目」的
  选择器 UI。决定要做的话这是一整块独立工作。
- Classic 菜单（`ui/clickgui/Menu.cpp`）的 `PropConfigurerElement` 只有
  CheckBox / Slider / ComboBox 三种实现，`BindProperty` 和 `ColorProperty`
  会走跳过分支。要在 Classic 里也能绑键、改颜色，得仿照 Panel 那侧的
  `BindElement` / `ColorSelectorElement` 补两个 `PropConfigurerElement`。

**代码卫生**：

- `Client/README.md` 过时了 —— 提到 `src/utils/keyboard.h`（实际是 `src/utils/Keyboard.h`），
  也没反映现在的面板 UI
- `Client/sources.txt` 只有 8 行，重复且过时
- `Client/test/bytecode/` 里有 `patch_test.cpp` 和 `build/` 下的 `.obj` / `.exe`，
  这个 cpp 也没接进 CMake
- `Client/` 下堆了好几个废弃构建目录：`build/`、`build-verify/`、`build-vs2026-final/`、
  `build-vs2026-minecraft/hooks/`、`out/`
- `src/minecraft/modules/combat/example/Example.cpp` 会随产品一起注册发布，要么删要么从 `Registry.cpp` 摘掉
- `src/minecraft/java/game/GameInput.cpp` 和 `GameOptions.cpp` 里有一套重复的 `Handles` +
  `g_failureLogged` 模式，可以抽出来

**功能**：`docs/event-targets.md` 那张表里大部分事件还是「—」（目标定了、规则没写）。
那张表末尾的「注意」一节写了 Forge 运行时会二进制补丁类，匹配时要考虑这点。

## 8. 排查手法备忘

- **原生崩溃**：`latest.log` 没有异常就说明不是 Java 异常，去看 `hs_err_pid*.log`。
  里面 ExceptionInformation 的第一个字是异常码（`0xe06d7363` = C++ 异常），
  然后是 ThrowInfo 指针、对象指针、模块基址。
- **反汇编**：`dumpbin /disasm` 产物约 20 万行，用脚本过滤 RVA 区间比手翻快。
  PE 布局：`.text` VA=0x1000、`.rdata` VA=0xa7000、`.data` VA=0x1161000。
  构建产物没开 `/MAP` 了，要符号就临时加回 `LINK_FLAGS "/MAP"`。
- **坑**：`/OPT:ICF` 会把相同函数折叠，`.map` 里「最近的前一个符号」这种归属判断**不可靠**，
  不能只看映射就下结论。
- **MSVC STL**：`std::string` 里 `_Bx` 在 +0（16 字节），`_Mysize` 在 +0x10，`_Myres` 在 +0x18，
  共 32 字节，SSO 容量 15。`assign(const char*, size_t)` 是若干重载的公共落点。
- **按键码两套 API 不一样**：`GetAsyncKeyState` / `GetKeyState` 认
  `VK_RSHIFT`(0xA1) 这种左右专用码，键盘**消息**不认——消息里 Shift/Ctrl/Alt
  恒为中性码。把轮询改成 WndProc 分发时最容易踩这个：绑定查 `161`，消息永远
  是 `16`，表现就是「按键完全没反应」。
- **本机 shell 怪癖**（Git Bash）：复杂引号的单行命令会间歇报
  `unexpected EOF while looking for matching '`。绕法是写 `.py` / `.sh` 到
  `%TEMP%` 再执行。
