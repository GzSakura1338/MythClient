# Minecraft 26.2 分析参考

- `src/`：Fabric Loom 1.18.2 使用 Vineflower 从 26.2 合并客户端/服务端 JAR 生成的源码，共 7055 个 Java 文件。不是 Mojang 发布的原始源码。
- `minecraft-26.2-client.jar`：Mojang 官方客户端发布包，可供 Recaf 导入。
- 官方客户端 SHA-1：`2dc72797acbc1b63fc16a11c4ac393605f453754`，已校验一致。

这些文件仅作源码和字节码分析参考，不参与 Myth 编译。
Loom 已完成源码生成；最后写入调试行号映射时，被正在运行的 Minecraft 文件锁阻挡，因此 `genSources` 任务整体报告失败。
