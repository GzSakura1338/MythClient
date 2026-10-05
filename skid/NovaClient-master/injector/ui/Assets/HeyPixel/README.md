# HeyPixel 进服后加载适配资源

适配的官方原 JAR SHA-256：`BD500EA2DD0E2806450A252671D406336A93A3FCD6A076D63E2B55BE196DE192`。

`loader.class.bin` 来自 `skid/DEOBF/heypixel-nova/loader-join.class`。Recaf MCP 确认原类有一个空的、已标注 SubscribeEvent 的 `ClientPlayerNetworkEvent.LoggingIn` 方法；补丁直接在该方法返回前调用加载 helper，不新增运行时 Java 类，不修改原生库加载入口。

helper 由 Recaf MCP 组装（`nova-loader-join.asm`）。它排除单人世界及缺少玩家/世界的状态，读取 `/lib/NovaClientBase.dll`，写入临时文件并 `System.load`，用同步布尔标记防止重复加载。Java 层加载错误只记日志，不向 Forge 事件派发链传播。进服前不调用 helper，不提前加载 DLL 或连接 JNI/JVMTI。

`RestoreFrames.java <stage> join` 从原版类恢复全部原方法，只移植 Recaf helper、标记字段及一次进服回调调用，避免 Recaf 重算无关方法栈帧造成 VerifyError。

加载类 SHA-256：
- 原类：`8E5432C11F78EFED7ACAE58C15118C1FA04C2555E1FA88825B598469C035831C`
- 旧预加载补丁：`592FDCBAA23F3B6C9C816E7EAD511B87B92293EF2D280DE6B7FAB4B09DCB83CA`
- 当前进服补丁：`43EB1EFEB6CD46D2566D12DC8D7D189A91EDC9D380925C63A7B05D54EF05BAB0`

`VerifyPreload.java <stage> <dll> <jar> join` 校验所有原指令、栈帧、注解和无关条目，原生库加载入口必须与原版一致。`Client/test/loader/heypixel-deferred.ps1 -JoinLoader <loader.class.bin> -Dll <dll>` 使用隔离 JVM 执行精确 helper/回调。真实服务器兼容性需要游戏启动验证，不能由这些测试保证。
