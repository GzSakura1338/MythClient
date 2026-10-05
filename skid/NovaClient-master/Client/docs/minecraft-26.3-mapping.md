# Minecraft 26.3 mapping 分析

## 来源与范围

仅通过 Recaf 当前工作区执行只读查询，没有打开、切换或附加其他 JAR，也没有修改工作区字节码。

- 当前主资源：`D:\AI\VibeCoding\shader\skid\minecraft-26.3-client.jar`
- SHA-256：`4508D006323F24FA02876310C192D739AF56516EB259000AC50F0909A68C9A2D`
- `version.json`：版本 26.3、协议 777、Java 25、构建时间 2026-09-15T11:20:48+00:00。
- `Minecraft` 的 class-file major 为 69。
- 类与成员使用可读名称；这份文件描述当前客户端，不能据此认定为 Forge 26.3。
- 输出：`Client/src/minecraft/compat/mappings/minecraft263.inc`。包含 188 个类、1212 个字段、1667 个方法/构造器及 11 个 Hook 候选方法边界。
- 覆盖 Nova 涉及的输入、实体、世界、容器、物品、网络、皮肤和渲染接口及其新版本替代类型。不是整个 Minecraft JAR 的全量符号导出，也不是 Forge 1.20.1 常量的一对一重命名表。

Minecraft/RenderPearl 映射的声明类、名称、描述符及 static 标记来自 Recaf 成员查询。Vulkan 接入另使用 LWJGL 的 `Pointer.address()`、`VkDevice.getPhysicalDevice()`、`VkPhysicalDevice.getInstance()`：当前 JAR 不包含这些库类，其签名对照 LWJGL 上游源码，并通过运行时 JNI 解析验证。继承成员记录在实际声明类；例如 `ItemStack` 的 tag 判断还需查看 `TypedInstance` 接口。不存在的旧成员没有伪造占位。

## 如何使用

文件采用和 Lunar 一样的五种 X-macro：

```cpp
#define NOVA_CLASS(id, name) /* class lookup */
#define NOVA_FIELD(id, owner, name, desc) /* instance field lookup */
#define NOVA_STATIC_FIELD(id, owner, name, desc) /* static field lookup */
#define NOVA_METHOD(id, owner, name, desc) /* instance method lookup */
#define NOVA_STATIC_METHOD(id, owner, name, desc) /* static method lookup */
#include "minecraft/compat/mappings/minecraft263.inc"
// 随后 undef 以上宏。
```

类声明集中在成员声明前，方便分两阶段解析。字段 ID 带 `Field_`；重载方法以 `_2`、`_3` 等区分，调用时必须依据描述符选取。定义 `NOVA_MC263_INFO` 可读取版本元数据；定义 `NOVA_MC263_HOOK(id, owner, name, desc)` 可读取 Hook 候选方法列表。不要给此文件添加 include guard。

文件已接入 `ClientProfiles.inc` 和注入器。26.3 profile 的直接 screen 字段留空，由适配器读取 `Minecraft.gui -> Gui.screen()`。定义 `NOVA_MC263_EDITS` / `NOVA_MC263_BINDINGS` 分别生成实际改写规则和 native 绑定；候选方法目录与实际安装的规则分开保留。

## 已核实的重要变化

| 主题 | 26.3 实际声明/行为 | Nova 适配要求 |
|---|---|---|
| Minecraft 入口 | `getInstance()Lnet/minecraft/client/Minecraft;`，`player`、`level` 字段仍在 | 名称可直接解析，不能沿用 SRG 名 |
| 当前界面 | Minecraft 的 `gui:Lnet/minecraft/client/gui/Gui;` → `Gui.screen()Lnet/minecraft/client/gui/screens/Screen;` | 改成两步访问；Minecraft 不再声明 screen |
| 界面设置 | `Gui.setScreen(Screen)V` | 更新打开/关闭界面的适配 |
| 输入 | LocalPlayer.input 是 ClientInput；keyPresses 是 player.Input，moveVector 是 Vec2 | 不再写旧 Input.forwardImpulse/leftImpulse；Input 构造器是七个 boolean |
| 输入更新 | `KeyboardInput.tick()V` 创建 Input，再生成归一化 Vec2 | 移动修正需在新输入结构上实现；记录字段为 final 的类型应重建实例 |
| 移动发包 | `LocalPlayer.sendChanges()V` → `sendPosition()V` | Minecraft.tick 中仍会调用 sendChanges，必须重新选取事件时序 |
| 移动包构造 | PosRot 提供 `(Vec3;FFZZ)V`；Rot 为 `(FFZZ)V`；StatusOnly 为 `(ZZ)V` | 增加 horizontalCollision，不能照搬旧构造器 |
| 攻击 | MultiPlayerGameMode.attack 创建 `ServerboundAttackPacket(I)V` | 不再以 Interact 包里的 ATTACK action 判定攻击 |
| 挥手 | LivingEntity.swing 接受 `(InteractionHand;SwingAnimation;Z)Z`；另有 swingAndResetAttackStrength | ServerboundSwingPacket 在当前工作区不存在；须按新行为适配 |
| 容器点击 | `handleContainerInput(IIILnet/minecraft/world/inventory/ContainerInput;Lnet/minecraft/world/entity/player/Player;)V` | ClickType 替换为 ContainerInput；PICKUP/QUICK_MOVE/SWAP/THROW 字段已核实 |
| 背包选中槽 | `Inventory.getSelectedSlot()I`、`setSelectedSlot(I)V` | 使用访问器；旧 armor 列表已改为装备结构 |
| 物品识别 | SwordItem、AxeItem、PickaxeItem、ShovelItem、ArmorItem 等旧类不存在 | 使用 ItemTags、DataComponents、装备/工具组件；不能继续 instanceof 旧类 |
| 附魔 | Enchantments 字段为 ResourceKey；相关组件/Holder 接口已收录 | 旧静态 Enchantment 对象及固定签名不能直接复用 |
| 标识符 | `net.minecraft.resources.Identifier` | 替代旧 ResourceLocation 类型 |
| 击退包 | `ClientboundSetEntityMotionPacket.id()I`、`movement()Lnet/minecraft/world/phys/Vec3;`；构造器 `(ILnet/minecraft/world/phys/Vec3;)V` | 不再按三个整数字段读取并除以 8000 |
| 应用击退 | ClientPacketListener.handleSetEntityMotion 调用 Entity.lerpMotion(Vec3)V | AppliedVelocity 事件应在新的实际应用位置适配 |
| 网络发送 | Connection.sendPacket(Packet, ChannelFutureListener, boolean)V | 旧 PacketSendListener 签名不匹配 |
| 实体插值 | xOld/yOld/zOld；fallDistance 类型为 double | 不能沿用旧 float 字段 JNI 访问 |
| 皮肤 | AbstractClientPlayer.getSkin → PlayerSkin.body → ClientAsset.Texture | 皮肤标识符访问链改变 |
| 纹理 | AbstractTexture.getTexture 返回 RenderPearl GpuTexture | 不能继续当 OpenGL texture ID 使用 |
| 窗口 | Window.createWindow 调用 GpuBackend.createWindow 与 SDLVideo | 旧 GLFW/LWJGL2 的交换缓冲假设不成立 |
| 渲染 | GameRenderer.update/extract/render 分开；Minecraft.renderFrame 最后调用 GpuSurface.present | HUD 绘制接入和 GPU 资源管理需另做适配 |
| 实体渲染 | LivingEntityRenderer.extractRenderState 填写 LivingEntityRenderState，再由 submit 消费 | 不能直接使用 1.20.1 render(Entity, ...) 的注入点 |

部分实体仅移动包名：Slime → `monster.cubemob.Slime`；AbstractHorse → `animal.equine.AbstractHorse`；WaterAnimal → `animal.fish.WaterAnimal`；AbstractVillager → `npc.villager.AbstractVillager`；EnderDragonPart → `boss.enderdragon.EnderDragonPart`。新名称已经由类目录和成员元数据核实。

位置历史区分逻辑用 `xo/yo/zo` 与 `EntityRenderer.extractRenderState` 使用的 `xOld/yOld/zOld`，NameTags 按后者插值；partial tick 使用与世界提取一致的 `getGameTimeDeltaPartialTick(false)`，包含冻结世界语义。

## Hook 候选点与证据

已读取以下方法的实际字节码，而非仅根据名称推测：

- Minecraft.tick、handleKeybinds、renderFrame、setScreenAndShow。
- LocalPlayer.aiStep、sendChanges、sendPosition。
- KeyboardInput.tick。
- MultiPlayerGameMode.attack、useItem。
- ClientPacketListener.handleSetEntityMotion。
- LivingEntityRenderer.extractRenderState(LivingEntity, LivingEntityRenderState, float)。
- Window.createWindow。

输入证据：LocalPlayer.aiStep 实际调用 ClientInput.tick；KeyboardInput.tick 写入 keyPresses 和 moveVector。
旋转证据：sendPosition 通过 getYRot/getXRot/onGround 和 horizontalCollision 构造新的移动包。
交互证据：handleKeybinds 先处理使用物品状态，再消费攻击/使用键；可作为 PrePlayerInteract 候选边界。
渲染证据：extractRenderState 读取 yHeadRotO/yHeadRot 和 getXRot(float)，写入 bodyRot/yRot/xRot。
网络候选 sendPacket 和 channelRead0 的重载及描述符已核实；线程、取消与重入语义仍需实现时验证。

`NOVA_MC263_HOOK` 仍仅为候选目录；实际 `EDITS` / `BINDINGS` 已覆盖 11 个游戏类、22 个 native 绑定，详见下文。离线校验通过不等于已在用户的游戏中安装成功。

## 验证边界

26.3 的 `FirstPersonHandsAndItems` 可能在注入时尚未加载。安装前通过游戏类加载器加载全部 15 个目标类（不运行静态初始化），随后统一重转换并严格核对规则。不能直接将当前已加载类数与完整规则表比较，否则会出现 14 个类改写成功但安装被撤销。`test/minecraft263/run_lazy_targets.ps1` 在独立 JVM 中验证延迟加载、实际钩子执行、卸载还原，以及缺类/规则失败时继续拒绝安装；测试用字节码由 C++ 生成。

`test/minecraft263/audit_mapping.py` 对同一 JAR 核对所有 Minecraft/RenderPearl 成员的声明类、名称、描述符和 static 标记；外部 LWJGL 成员单独统计。事件离线测试使用生产改写器处理真实 class-file 69，再用 JDK ClassFile verifier 检查改写结果和类型层次，不启动 Minecraft。

## 已接入的注入路径（2026-09-18）

- 注入器共享目录新增 ID 3 / `minecraft-26.3`，界面显示 `Minecraft 26.3 (OpenGL / Vulkan)`。
- 同一个 DLL 通过 `Minecraft263.cpp` 解析本文件中的名称。先校验 `SharedConstants.getCurrentVersion().id()` 为 `26.3`，再根据 RenderSystem 的实际 backend 名称选择 OpenGL 或 Vulkan。
- 当前 Recaf 中的 `PreferredGraphicsApi.getBackendsToTry()` 默认优先 OpenGL；显式选择 Vulkan 则优先 Vulkan。Nova 自动跟随实际后端，不修改游戏的图形 API 设置。
- `GlSurface.present()` 实际调用 SDL3 的 `SDL_GL_SwapWindow`。新增该导出的原生 Hook，只处理 Minecraft 主 SDL 窗口，在默认绘制 framebuffer 上绘制 Nova，再调用原函数并保留 bool 返回值。
- Vulkan 从 `Minecraft.windowSurface -> FrontendGpuSurface.backend -> VulkanGpuSurface.device` 获取逻辑设备、物理设备、实例和图形队列；通过公开的 `vkGetDeviceProcAddr` 解析并 Hook 实际 device dispatch 的 present/create/destroy 函数，兼容 LWJGL 缓存的函数指针。
- 原生窗口通过 SDL3 的 `SDL_GetWindowProperties` / `SDL_GetPointerProperty("SDL.window.win32.hwnd")` 获取。Vulkan 复用 Win32 输入与 ClickGUI 平台层，完全不访问 OpenGL 上下文或 GL 纹理 ID。
- 26.3 的成功回执等待首个 Nova 帧；仅创建 Hook 不报告成功。注入时保持游戏窗口正常绘制。若首帧超时且卸载日志显示未完成，重启游戏后再试。
- 当前界面通过 `Minecraft.gui -> Gui.screen()` 查询；聊天栏防误触沿用 ClickGUI 入口规则。鼠标释放/恢复走 26.3 的 MouseHandler；游戏界面打开时不会强制重新抓取鼠标。
- 共用完整 23 模块注册表及其参数界面，不再使用五模块预览列表。配置目录为 `%USERPROFILE%\.nova\configs\minecraft-26.3`。
- 实体、射线、移动、攻击、挥手、方块放置、选槽、物品组件、附魔、背包/箱子容器、网络、计分板、相机矩阵和皮肤均通过 26.3 适配器接入公共功能。FullBright 包含 Gamma/NightVision。
- AutoBlock 开关和参数保留；现代版本不执行 1.8.9 剑格挡、不模拟盾牌，开启后不会拦截 KillAura 普通攻击。版本机制差异不作强行统一。
- Tick、玩家 tick、移动输入、移动包朝向/地面标志、跳跃/移动/攻击朝向、实体和物品射线、第三人称渲染状态、入站/出站包、实际击退应用均已接入。必须全部改写成功才启用运行；出站重入使用 Win32 TLS，卸载在桥接静止后释放。
- 26.3 主手挥手发送 `ServerboundPunchPacket.INSTANCE`，攻击走独立 `ServerboundAttackPacket`。速度包 Vec3 仅在适配层转换为公共接口的 8000 单位定点值。PosRot 保留 `horizontalCollision`。
- 皮肤从 NativeImage 读取 CPU 像素，通过 ImGui 用户纹理交给当前 OpenGL/Vulkan 后端上传；HUD 不再把 26.3 的 RenderPearl 纹理当作 GL texture ID。保留像素以便交换链重建后重新上传。

构建产物：

- `Client/build-vs2026/Release/NovaClientBase.dll`
- `build/Injector.WinUI.exe`（使用所在目录内的依赖）
- `build/NovaInjector.exe`

CLI：`NovaInjector.exe <pid> <absolute-dll-path> minecraft-26.3`。

验证：DLL Release 构建通过；本次未改注入器。`Client/test/minecraft263/run.ps1` 用真实 JVM、`-Xcheck:jni` 和 Java fixture 测试实际初始化及 JNI 适配代码，覆盖版本/后端拒绝、Vulkan 句柄、SDL、屏幕/鼠标、Sprint、Gamma、Vec3 击退读取、PosRot 重建、容器过期拒绝、QuickMove、背包/护甲槽边界、入站回放/线程交接异常及 2000 轮引用生命周期。`patch_hooks.cmd` 检查 11 个游戏类的 35 项改写/结构断言，并用 JDK 校验包含桥接类在内的 12 类、1959 个方法。fixture 与独立 GPU 测试不代替游戏内功能验收；注入及实际操作由用户执行。

`Client/test/opengl/run.cmd` 使用独立隐藏 WGL 窗口验证皮肤纹理上传、实际像素、背景保留以及 OpenGL 后端重建。Vulkan 测试也包含相同皮肤缓存路径，检查首次上传、交换链缩放及卸载后重新接入的纹理像素。两种后端均在 NVIDIA GeForce RTX 5060 Laptop GPU 上通过。

## Vulkan 交换链与同步

`VulkanGpuSurface.configure()` 原本只创建 `VK_IMAGE_USAGE_TRANSFER_DST_BIT` 交换链。因此不能直接把 ImGui render pass 绑定到已经存在的图像。首次呈现旧交换链后，Nova 将成功结果转成 `VK_SUBOPTIMAL_KHR`，让 Minecraft 已有的 `isSuboptimal -> configure` 分支在自己的渲染线程重建。创建 Hook 仅对目标 surface 增加设备支持的 `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`，保留其他参数。

`VulkanOverlay.cpp` 创建独立 render pass（LOAD 原画面，STORE 输出，PRESENT → COLOR_ATTACHMENT → PRESENT）、image view、framebuffer、command pool、fence 和 semaphore。它不拥有或销毁游戏的 VkDevice、VkQueue、VkSurface 或交换链图像。

呈现时，Nova 的提交等待游戏原本交给 present 的信号，绘制后发出自己的信号，再由 present 等待它；同一个游戏二进制 semaphore 不会被等待两次。只有提交成功才替换 wait 列表，保留原 `pNext`、`pResults` 和驱动返回值。完成信号按交换链图像分配，CPU 命令及 ImGui buffer 按同长度的帧环配 fence。缩放、VSync 切换导致交换链重建时，等待图形队列完成后释放旧资源，保留 ClickGUI 平台和配置状态，再重建 renderer。

收到 DEL 后在呈现线程等待 GPU、释放 Nova 的资源、恢复鼠标/WndProc、禁用三个原生 Hook；随后沿 DLL 原有线程静止检查卸载。Vulkan 成功回执只在 Nova 提交成功且原 present 成功或返回 SUBOPTIMAL 后发出。

`Client/test/vulkan/run.cmd` 在独立隐藏 Win32 窗口运行真实 Vulkan/ImGui 后端与同一份 Hook 代码。已在 NVIDIA GeForce RTX 5060 Laptop GPU 上验证 GPU 像素回读（红色叠加块及未被覆盖的背景）、已有交换链接入、256×192 → 320×240 重建、连续 120 帧、卸载和再次接入。测试回读代码仅在 `NOVA_VULKAN_TEST` 编译时启用，不进入发行 DLL。该测试不是 Minecraft 实例，也不代表已覆盖所有厂商驱动；实际游戏注入、Right Shift/ESC 和 DEL 操作交由用户验证。
