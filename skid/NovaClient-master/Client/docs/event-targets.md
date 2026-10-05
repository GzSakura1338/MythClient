# 事件触发点对照表（Starain 对齐）

运行时是 SRG 命名的 Forge 1.20.1：类名已反混淆，成员名是 `m_12345_` / `f_12345_`。
名字不是猜的，是两份映射拼出来的：

- `libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-mappings.txt`
  —— Mojang 官方映射，可读名 → 混淆名（`Entity` → `bfj`，`attack` → `d`）。
- `libraries/de/oceanlabs/mcp/mcp_config/1.20.1-20230612.114412/mcp_config-…-mappings.txt`
  —— MCPConfig CSV，混淆名 + 混淆描述符 → SRG（`bfj` 段里 `d (Lbfj;)V` → `m_5706_`）。

单独任何一份都不够：官方映射止于混淆名，CSV 从混淆名开始。**按名字+描述符**拼接才得到
`Player.attack` = `m_5706_`。只按名字拼会把继承成员配错（`attack` 曾配到 `f_150091_`）。

重建脚本：把两份文件读进来做笛卡尔拼接，产物见下表的 SRG 列。

## 表

「挂上」一列是**已经在 `src/minecraft/hooks/Edits.cpp` 里、真的会生效**的；「—」是目标已经定下来、
规则还没写进表里的。表的其余部分对两种都成立。

| 事件 | 目标方法 | 注入 | 挂上 |
|---|---|---|---|
| GameTickEvent PRE | `Minecraft.m_91398_()V` | HEAD 追加（零操作数） | ✅（POST 未挂） |
| PlayerTickEvent | `LocalPlayer.m_8119_()V`，方法入口 | 前插 + 取消（`Guard`） | ✅ |
| MotionEvent PRE / POST | `LocalPlayer.m_108640_()V` | HEAD / TAIL 追加 + 改写 3 处取值 | ✅ |
| RespawnEvent | 同上，`tickCount <= 1` | 同 MotionEvent | — |
| SlowdownEvent | `LocalPlayer.m_8107_()V` → `isUsingItem()` `m_6117_()Z` | 调用点改写 | ✅ |
| JumpEvent | `LivingEntity.m_6135_()V` → `getYRot()` `m_146908_()F` | 调用点改写 | ✅ |
| FallFlyingEvent | `LivingEntity.m_7023_(Vec3)V` → ⚠ **此方法里根本没有角度调用** | 目标待定 | — |
| RotationAnimationEvent | `LivingEntity.m_5632_(FF)F` → `getYRot()` `m_146908_()F` | 调用点改写 | ✅ |
| AttackYawEvent | `Player.m_5706_(Entity)V` → `getYRot()` `m_146908_()F` | 调用点改写 | — |
| AttackSlowdownEvent | `Player.m_5706_(Entity)V` → `setDeltaMovement(Vec3)` `m_20334_` / `setSprinting(Z)` `m_6858_` | 调用点改写 ×2 | — |
| StrafeEvent | `Entity.m_19920_(F;Vec3)V` → `getYRot()` `m_146908_()F` | 调用点改写 | ✅ |
| StuckInBlockEvent | `Entity.m_7601_(BlockState;Vec3)V`，`m_183634_` 调用点之前 | 插入 + 写回局部 2 | ✅ |
| RayTraceEvent | `Entity.m_20252_(F)Vec3` → `m_20171_(FF)Vec3` | 调用点改写（见下） | ✅ |
| UseItemRayTraceEvent | `Item.m_41435_(Level;Player;ClipContext$Fluid)` → `getYRot()` / `getXRot()` | 调用点改写 ×2（静态方法） | — |
| PacketEvent INCOMING | `Connection.channelRead0` → `genericsFtw` `m_129517_(Packet;PacketListener)V` | 调用点改写 | — |
| PacketEvent OUTGOING | `Connection.m_243124_(Packet;PacketSendListener)V` → `sendPacket` `m_129520_` | 调用点改写 | ✅ |
| PositionItemEvent | `MultiPlayerGameMode.m_233721_(Player;InteractionHand)` → `ClientPacketListener.send(Packet)` | 调用点改写 | — |
| ChatEvent | `ClientPacketListener.m_246175_(String)V` | HEAD + 取消 + 换消息 | — |
| ClickEvent | `Minecraft.m_91279_()V`，`LocalPlayer.isUsingItem()` `m_6117_` 调用点之前 | 前插 + 取消 | — |
| RenderWorldEvent | `GameRenderer.m_109089_(FJLcom/mojang/blaze3d/vertex/PoseStack;)V` | HEAD | — |
| RenderSkiaEvent | `Gui.m_280421_(GuiGraphics;F)V`，`renderCrosshair` 之后 | 后插 | — |
| MoveInputEvent | `KeyboardInput.m_214106_(ZF)V` | TAIL 追加，**零操作数**：钩子自己用 JNI 取 `Input` | ✅ |
| KeyboardEvent / MouseButtonEvent | 原生窗口过程 | 已实现 | ✅ |

已挂上的是 **10 个类、23 条规则**（16 条改写 + 1 条插入 + 1 条出口拦截 + 5 条追加）。其余各行的目标都是
从真实字节码读出来的，缺的只是 `Edits.cpp` 里的一行和 `Hooks.cpp` 里对应的那个 native。

`moveInput` 为什么是零操作数：TAIL 追加落在 `m_214106_` 最后的 `return` 上，而那个位置的
StackMapTable 只有一个 `chop` 帧（局部变量表为空），`aload_0; iload_1; fload_2` 拿去给校验器
就是 `JVMTI_ERROR_FAILS_VERIFICATION`（error 62），整个类加载不起来。钩子改成不吃参数、
用 `Minecraft.getInstance().player.input` 自己把 `Input` 取出来，注入体就只剩一条
`invokestatic`。代价是每个 tick 多几次 JNI 调用——`m_214106_` 每 tick 只跑一次，可以接受。

## 用来辨认的字节码指纹
- `Entity.m_146908_()F` = `getYRot()`（读 `f_19857_`）；`m_146909_()F` = `getXRot()`（读 `f_19858_`）。
- `Entity.m_5675_(F)F` / `m_5686_(F)F` = 带插值的 `getYRot(float)` / `getXRot(float)`。
- `Entity.m_20171_(FF)Vec3` = `calculateViewVector`；`m_20015_(Vec3;FF)Vec3` = `getInputVector`。
- `Entity.m_19920_(F;Vec3)V` = `moveRelative`，其中 `getInputVector` 的**第 2 个 float 参数就是偏航角**。
- `Connection.m_129520_` 才真正走网络；`m_243124_` 是带队列的 `send`。
- `ClientPacketListener.m_246175_` = `sendChat`，`m_246623_` = `sendCommand`（映射脚本在这里
  回退错了，按字节码核对出来的）。

## 注意
Forge 在运行时会二进制补丁这些类，`ClassFileLoadHook` 拿到的是补丁后的字节。所以一切匹配
都按**名字 + 描述符**做，绝不按调用点序号硬编码偏移。

## 改写器的硬约束（`ClassFileEditor`）

写进表里的每条挂接都要满足这些，否则改写会被**拒绝**（类是原样，不会改成半截）：

- **调用点改写要求两侧栈效果完全一致。** `invokevirtual A.f()F` 换成
  `invokestatic H(A)F` 之所以安全，是因为两者都吃掉 1 个 A 槽、压入 1 个 F 槽。不一致就拒绝。
- **`appendBeforeLastReturn`（TAIL）要求方法只有一个返回。** 有提前返回的方法里，走那条路的
  调用方会绕开 hook —— 事件看起来"没触发"，和"游戏没做这件事"没法区分。多返回直接拒绝。
  顺带校验返回指令的操作码和描述符的返回类型对上，防止把 hook 装进同名的另一个重载。
- **插入长度会对齐到 4 字节**（补 nop）。`tableswitch` / `lookupswitch` 的 padding 相对于
  code 数组起点，插入长度不是 4 的倍数会改变它后面所有指令的地址。
- **方法体超过 65535 字节就拒绝**（`code_length` 是 u4，但 JVM 不收，且异常表用 u2 存偏移）。
- **调用点改写不改变 code 数组长度**，所以分支、异常表、StackMapTable 一个字节都不用动。
  插入则要平移这些，`ClassFileEditor` 会一起处理。
- **`insertCancelGuard`（`Guard`）还要额外满足栈帧那条**，见下。

## 出口拦截（`Guard`）：`PlayerTickEvent` 为什么能取消整个 tick

Starain 的 `Stuck` 要的是"这一 tick 直接不做"，对应 Mixin 的 `@Inject(cancellable)` +
`ci.cancel()`。改写做不到这件事：改写只能替换某一次调用，而"整个方法不执行"没有对应的
调用点。所以 `ClassFileEditor::insertCancelGuard` 在**方法入口**插：

```
aload_0
invokestatic Bridge.playerTick(Lnet/…/LocalPlayer;)Z
ifne <最后的 return>              // 补 nop 对齐到 4 字节
<原方法体>
```

也就是 `LocalPlayer.m_8119_()V` 里 `AbstractClientPlayer.m_8119_()` 之前**整整 173 字节
全都跳过**：`super.tick()`、旋转包、`ServerboundPlayerInputPacket`、还有那个遍历
`f_108593_` 的环境音循环。这一点是有意的——只拦 `super.tick()` 会留下旋转包和输入包
在"冻结"期间照发，和服务端对不上。

**约束是那个跳转的目标必须"接得住"入口的栈帧。** 入口处活着的局部只有 `this` 和参数；
目标指令的 StackMapTable 帧是编译器按方法**中段**的活跃变量算出来的，两者不一定一致：

- 帧里某个槽是 `top`，入口处是实类型 —— **可以**。`top` 在目标一侧接受任何东西。
- 帧里某个槽是实类型，入口处是 `top` —— **不行**：

  ```
  Type top (current frame, locals[1]) is not assignable to '[I' (stack map, locals[1])
  ```

  整个类加载不起来。所以 `insertCancelGuard` 在改之前先把最后一帧的局部变量表解出来，
  槽位号 ≥ 参数槽数（`this` + 参数）的活局部只要存在一个就**拒绝**。

`m_8119_` 恰好是允许的那种：它最后的 `return`（偏移 173）的帧是 `chop` 出来的
`locals = [top]`，一个实局部都没有。验证台里 `guardedTick` / `guardedTickArg` 是允许的形状，
`guardedTickBusy`（在方法内部声明数组，于是那一帧带着一个入口处不可能有的槽）是被拒的形状——
三条都在 `defineClass` 下真跑过。

同一课的另一半：**`top` 只在一个方向上宽松**，靠"看起来大概行"去推是推不出来的。
`m_8119_` 这条是先怀疑、再拿真实类定义一次才敢下的（`.tmp_mcp` 里的 javap 只说了帧长什么样，
没说校验器收不收）。验证台就是为这类问题存在的。

离线验证台：`Client/test/bytecode/`，`./run.sh` 一把跑完。它用 `test/bytecode/classes/` 里那批
从 srg jar 抽出来的真类跑**生产用的那张规则表**（`src/minecraft/hooks/Edits.cpp` 是纯数据，直接链进验证台，
所以表里加一行就是验证台多跑一行），再做独立结构审计；另外用三个自造的 `Probe` 类做
**加载 + 执行**验证（`defineClass` 会强制 HotSpot 校验器跑）：`Probe.patched` 走全部改写和
插入形状，`Probe.carried` 单独验写回局部的那个操作码选对了类型，`Probe.guarded` 单独验
出口拦截——包括它**该拒**的那一种形状。

验证台里唯一刻意重复的是钩子名和描述符那一小张表：改写器只按名字往常量池里写引用，
**从不检查那个方法存不存在**（它也没法检查——改写发生时钩子类可能还没定义出来），
所以两张表分头写、互相核对，才能发现"表和表对不上"。

**帧类型 247 那一课**：`Minecraft.class` 一进验证台，`m_91383_(Z)V` 的 StackMapTable 就报
"frame at 65731, not an instruction boundary"——而那个方法的 `code_length` 只有 1119。
两个独立的读法（`audit.h` 的审计器和 `ClassEdit.cpp` 的 `shiftStackMap`）都漏了同一件事：
247 是 64..127 那一段的**扩展写法**，也一样带一个 `verification_type_info`。少读那三个字节，
后面整张表就从帧数据中间开始解，解出来的偏移看着像模像样。审计器那一份必须修，否则
`Minecraft` 加进夹具之后连**未改写**的类都过不了审计。

改写器那一份的后果更隐蔽：把帧数据当帧来读，窄帧又是一进一出，多数情况下字节被原样搬回去，
**输出和输入一模一样**——所以这一条是照着 `ClassEdit.cpp` 里另一个走帧的函数
（`insertCancelGuard` 用的那个，它一直是对的）补齐的，不是验证台抓出来的。验证台为此加了一条
字节级断言：插入 4 字节之后，`m_91383_` 的 StackMapTable 长度不变、且**只有第一个帧的 delta
那一个字节**不同。

## 两种改法：改写（`Redirect`）和插入（`Insertion`）

**改写**只换"哪个方法回答这次调用"，三个字节原地替换，所有偏移不动。
`invokevirtual A.f()F` → `invokestatic H(A)F`，栈效果必须完全一致，否则拒绝。

**插入**是"这件事取决于调用方拿返回值做了什么"时才用的。`Entity.m_7601_` 就是：

```
0: aload_0
1: invokevirtual m_183634_:()V
4: aload_0; aload_2
6: putfield f_19865_        // this.stuckSpeedMultiplier = vec3
9: return
```

乘数先是栈上的值，九个字节之后进了字段，**中间没有缝**能让改写插进去——事件要改的是方法
对这个值做了什么，不是某个调用返回了什么。所以 hook 放在那次调用**前面**，拿三个局部
（`this`、`BlockState`、`Vec3`），把答案**写回局部 2**，后面那句 `putfield` 自然读到新的。

放在 TAIL（`return` 前）不行：那时候字段里已经是原值了。

插入的字节是**推导出来的，不是写死的**：参数类型从方法的描述符读，槽位从参数宽度累加，
`long`/`double` 占两个槽所以"参数序号 ≠ 槽位号"。这三件事都由
`ClassFileEditor::insertHookCall` 算，所以签名里有 `long` 不是需要谁记住的特例。

它同时会**拒绝**两种组合，而且必须在改之前拒——这两种错误的结果是**整个类加载不起来**，
不是一个事件不触发：

- 返回值被丢掉（hook 有返回值但 `storeSlot = -1`），栈会比进来时深一格；
- 无返回值却要写回（`void` 的 hook 配 `storeSlot >= 0`），弹了一个从没压过的槽。

`maxStack` 同理是算出来的：插入开始时栈上已经有被顶掉的那次调用压的东西
（`m_7601_` 里是那个 `aload_0`），插入再往上堆自己的参数，最深处是
`callee 参数槽数 + hook 参数槽数` = 1 + 3 = 4。**`extraStack` 是 `max_stack` 的绝对下限而不是
增量**，所以这个数小一格就是 `VerifyError`。

> 这一条是踩出来的：`LocalTypeOf` 一开始读的是描述符的**最后一个字符**，而对象类型
> `Lnet/…/Vec3;` 结尾是 `;` 不是 `L`，于是 `Vec3` 被当成 int，发出了 `istore_2`。
> **结构审计抓不到这个**——它只认指令边界，不做类型检查。抓它的是 JVM 校验器，所以验证台
> 现在多了一个 `Probe.carried`：同样三个方法（返回引用 / float / int 各一个），
> 改完之后真的 `defineClass` 再真的调用，字段里必须是 hook 的答案而不是调用方传进去的那个。

## 三处"表"和它们各自的检查

| 表 | 位置 | 谁核对 |
|---|---|---|
| 钩子（名字 + 描述符 + 函数指针） | `src/minecraft/hooks/Hooks.cpp` 的 `kHooks` | `Install` 用 `stackEffect` 解析每个描述符 |
| 改写规则（哪一行、往哪改） | `src/minecraft/hooks/Edits.cpp` 的 `kEdits` | `Install` 的 `EditsAgreeWithHooks()` 逐行比对钩子表 |
| 真正干活的那张 | `src/minecraft/hooks/Transformer.cpp` | 运行时——它只走 `EditsFor()` |

`EditsAgreeWithHooks()` 是这三者之间唯一无法在下游自动发现的裂缝：改写后的调用点是一次
惰性解析的 `invokestatic`，名字写错了类照样加载，等到那行代码第一次跑才抛
`NoSuchMethodError`——读起来像"事件没触发"，也像"游戏崩了"，而且两种读法都不会说是哪张表
出的问题。

## 表对得上，问的类不一定对（`m_6117_`）

三张表互相一致，保证不了它们**问的东西存在**。`usingItem` 那一次就是：钩子表、改写表、
描述符全都对得上，改出来的字节也过了验证台和 JVM 校验器，但注入之后桥整个装不上。

```
Hooks: a game method or field did not resolve (yRot=1 xRot=1 viewVector=1 usingItem=0 yRotO=1 xRotO=1 zero=1)
Bridge: the hooks could not resolve what they stand in for
```

七项里六项是 1。`Hooks.cpp` 拿 `Entity` 去问 `m_6117_`，而 JNI 的 `GetMethodID`
**只查本类和父类，不查子类**——`Entity` 是 `LivingEntity` 的父类，`m_6117_` 声明在子类里
（映射里 `bfz` = `LivingEntity`），所以永远是 null。`javap` 一句话就能确认：`Entity` 里
`m_6117_` 出现 0 次。其余六项（`m_146908_`/`m_146909_`/`m_20171_`/`f_19859_`/`f_19860_`/
`f_82478_`）确实都在 `Entity` 上，所以只有这一项挂掉。

连带的一课是**现象离原因很远**：一个 null 的 `jmethodID` 让 `Resolve` 整体返回 false，
桥不装，`Loader.cpp` 的 `else if` 于是压根不会去调 `EnableTransformer`——看起来像
"transformer 没生效"，实际上一行 JVMTI 代码都没跑到。**日志里没有 `Transformer:` 开头的行，
就是这件事的指纹。**

## 改写是两半，缺一不可（`Transformer.cpp`）

- **`ClassFileLoadHook`** 管的是**之后**才加载的类。`LocalPlayer` 要等进世界才有，
  注入时它还不存在。
- **`RetransformClasses`** 管的是**之前**就加载好的类。`Entity` 在 Nova 进进程之前早就加载
  完了——只挂 hook 的话，表里一条都不会生效。

用 `RetransformClasses` 而不是 `RedefineClasses`：类保持身份不变，只有常量池和方法体动，
而这正好就是调用点改写会碰的全部；而且只有它允许**新增常量池项**，解析
`Bridge.viewVector` 需要一个。

几个不显然的点：

- 已加载的类按**签名**从 `GetLoadedClasses` 里挑（`Lnet/minecraft/world/entity/Entity;`），
  不是拿游戏类加载器去 `loadClass`。为了改写一个类而先把它加载出来是反的：主菜单里
  `LocalPlayer` 本来不该被初始化。
- `SetEventCallbacks` **整体替换回调表**，不是只填传进去的那几项。目前 Nova 只注册这一个
  回调；以后再加第二个 JVMTI 回调时必须从 `EnableTransformer` 走，否则会把这个悄悄顶掉。
- 钩子回调跑在**正在加载类的那个线程**上，下面没有我们的 Java 帧：不能抛异常出去，
  也不能假设自己是唯一的（两个线程同时加载两个类会同时进来）。
- 桥自己的那个类按名字跳过——它是我们刚用 `DefineClass` 定义出来的，表里没有它的规则，
  放它进来就是每次都把自己生成的字节再解析一遍。
- 某一条规则匹配不上**不中止整个表**（Forge 会二进制改这些类，改动了就是没验证过的版本），
  但会点名是哪一行；只有**一条都没匹配上**才原样放行。

## 拆下来：`DisableTransformer`

改写分两半装上去，拆也要拆两半，顺序还是反的：

1. **先关事件**：`SetEventNotificationMode(JVMTI_DISABLE, …)`。这不是保险措施而是机制本身
   ——**retransform 自己就是一次 `ClassFileLoadHook` 投递**，钩子还开着的话下一步只会把类
   再改写一遍，什么都还不回去。
2. **再清回调表**：关掉事件不等于 JVM 不会再碰这张表，而它马上就要指向一个正在退出进程的映像。
3. **最后再 retransform 一次**：还原。

第 3 步成立，是因为两个 JVMTI 重定义调用**不对称**：`RedefineClasses` 会更新"初始字节"，
`RetransformClasses` **不会**。所以第二次 retransform 拿到的是类的**原始字节**而不是当前
字节——这条性质正好就是还原手段。

位置在 `Loader.cpp`：`WaitForTeardown` 之后、`ImageIsClear` **之前**。改写后的调用点是
**一条跳进本映像的直接跳转**，所以只要还没还原，游戏线程随时可能进来；`ImageIsClear` 问的是
"此刻有没有人在里面"，只有把"得到答案"和"解除映射"之间的事缩到最短，这个答案才有意义。
**先还原，再排空，最后解映射。**

还原被拒绝时（`RetransformClasses` 对整个集合是全有全无，拒绝就是一个类都没动），卸载
**中止**，DLL 留下——和这个函数其它每一条放弃路径一样。留着一个还在被改写的映像，重开游戏
就好；解映射了，就是下一次移动时的崩溃。

## RayTraceEvent 为什么挂在 `m_20171_` 上

`m_20171_(float,float)Vec3` = `calculateViewVector`，是**偏航和俯仰第一次同时到手**的地方，
而且是所有“从眼睛射出去”的射线唯一的漏斗：`m_20252_`（pick）、`getViewVector`、`getLookAngle`
全走它。字节码：

```
public final Vec3 m_20252_(float);
   1: invokevirtual m_5686_:(F)F      // getXRot(partial)
   8: invokevirtual m_5675_:(F)F      // getYRot(partial)
  11: invokevirtual m_20171_:(FF)Lnet/minecraft/world/phys/Vec3;
```

改写 `m_20171_` 而不是上面那两个插值 getter，换来三件事：

1. **一次调用 = 一个事件**，`yaw` 和 `pitch` 在同一只手里。改写两个 getter 会每帧发两次
   事件，而且第二次拿不到第一次的结果。
2. **栈效果天然吻合**：`invokevirtual Entity.m_20171_(FF)Vec3` → `invokestatic H(Entity;FF)Vec3`
   都是吃掉 (Entity, F, F) 压回 Vec3，三字节改写，方法里所有偏移不动。
3. `m_20252_` 本身有多个出口，做不了 TAIL；`m_20171_` 不需要 TAIL。

## 钩子类的形状（`HookBridge`）

游戏里只多一个类，随机包名 + 随机类名，每次注入都不同：

- 全部方法都是 `public static native`，**一个字节的逻辑都没有**，只有签名。
- 用 `JNIEnv::DefineClass` 定义进游戏自己的类加载器（`JVMTI` **没有** `DefineClass`，
  也没有 `can_define_class` 这个能力——`src/third_party/jni/jvmti.h` 和 JDK 17 原版逐字节一致，
  可以拿来核对）。名字用**内部形式（斜杠）**，JDK 9 起不再接受点号。
- `RegisterNatives` 按 `src/minecraft/hooks/Hooks.cpp` 里的 `kHooks` 表逐个绑定。
- **表是三处唯一的事实来源**：建类、绑定、以及改写器往调用点里写的描述符都从它来。
  描述符在进 JVM 之前先过一遍 `ClassFileEditor::stackEffect`，写错了会点名是哪个钩子，
  而不是只报一句“类没加载起来”。
- 钩子的接收者一律声明成 `Entity`。调用点上的静态类型可能是 `LivingEntity`/`Player`，
  都能赋给 `Entity`，反过来不行——统一写宽的那个，就不会有一个钩子栽在校验上。

**改写的类加载之前，钩子必须先装好**：改写后的调用点是一次直接的 `invokestatic`，落在一个
还没解析出来的 `jmethodID` 上就是空指针。
