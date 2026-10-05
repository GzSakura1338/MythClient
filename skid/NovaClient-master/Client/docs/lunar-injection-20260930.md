# Lunar 1.21.11：2026-09-30 更新后的注入失败

## 已确认原因

`%APPDATA%/.minecraft/nova-base.log` 最新两次初始化报告四条 `required modern target could not be loaded`，对应账号页面、Xbox 账号刷新、菜单导航和屏幕事件类。Minecraft intermediary 核心接口、GLFW 和 48 个 native bridge 方法在此前均已初始化。

更新后样本为 `C:/Users/13366/.lunarclient/offline/multiver/cache/d1937f9/76a6f84/bake.zip`，旧样本为同目录下 `2b715c4b/bake.zip`。新样本于 2026-09-30 10:16 写入；Recaf MCP 加载了 17,866 个类。对账号 Handler、XboxAccount、Navigation、ScreenEvents 和 Manager 进行反编译及方法调用查询，确认账号逻辑仍存在，但 Lunar 类名、方法名已重新混淆。

Recaf 核实的新调用包括：

- 添加账号入口：`OCIRIOCRRRHRCOICCRIIRIIIORICCC()V` → auth utility `OOHIHIRCIIORRRRRCIRHIOOOROHIOH(Consumer)V`。
- 选择账号入口：`CCOCCCICCRHCHRIHCHIRHHCIRIICOC(String)String` → `IROICCOHOOHHOCCHCHOHIROCIOCCRI(String)Optional`。
- Xbox 刷新：`RCOOICHHOIIOIORIOROIOCOHIORCHR(Consumer)V` → auth utility 接收 Account 与 Consumer 的静态方法。
- 菜单导航依旧检查 `isRequiresAuthentication()`；多人屏幕事件仍调用账号管理器 `RIOIRIICROROICOHRROHIHCRHHICIH()Z`。
- 当前账号选择更新入口：`ROCIORICOCIHIOCCHRIIRHICORROHO(String)V`，继续更新 `selectedAccount` 模型。

## 修复

- 新增仅用于账号功能的构建映射，保留 09-27 的旧命名，加入 Recaf 核实的 09-30 命名。
- 事件重写和 CheckedJni 使用相同的映射，覆盖类名、方法名以及描述符；不是只替换四个类名。
- 启动时通过游戏 ClassLoader 预检整组账号钩子及关键账号内部方法。匹配新版或旧版后才将整组规则加入 Transformer。
- 未识别的账号结构跳过整个账号接管组，并在账号页面显示映射待更新；核心 Minecraft 事件仍保留严格检查。这个降级不放宽核心事件完整性要求，也不保证未知版本会话切换可用。
- 没有修改或导出 Lunar JAR；没有增加 Java 源码或 Java helper。

## 验证

- 新版完整事件测试：20 类、118 项检查、0 失败。
- 新版 JNI catalog：765 bindings、0 失败。
- 账号测试：五处重定向实际命中，重写后重新解析并校验分支/栈映射；20 个账号内部 JNI 方法存在性检查。
- 旧版账号映射回归与 Unknown 构建整组省略检查。
- Release DLL 构建及 reflective static TLS 检查。

尚未对运行中的游戏重新注入。日志中的 `inert image retained` 表示失败初始化保留了停用 DLL，随后重试已经出现 `Another Nova instance`。应完全退出并重新启动 Minecraft，再选择 `Client/build-vs2026/Release/NovaClientBase.dll` 注入。
