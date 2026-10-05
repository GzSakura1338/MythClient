# Lunar 入站包线程约定

Lunar 1.8.9 的入站 `PacketEvent` 在游戏主线程执行。Netty 收包后只创建原版 `PacketThreadUtil$1` Runnable、标记该任务，并调用 `Minecraft.addScheduledTask`。标记表锁在进入 Java 调度器之前释放。

原版 Runnable 的 `run()` 入口检查任务标记：

- 有标记：移除标记，执行入站模块回调；未取消时执行原版包处理器。原 Runnable 的后续代码不再重复执行。
- 无标记：保留原 Runnable 的完整行为。这涵盖注入前已排队的任务、原版创建的任务，以及模块通过 `HandleIncomingPacket` 在非主线程提交的回放。

必须按 **Runnable 身份** 标记，不能按 packet 身份标记；同一 packet 对象可能被回放。Java 异常继续由原版 `FutureTask` / `Util.runTask` 处理。

## 为什么不能在 Netty 上直接发布事件

实际 Lunar 字节码显示：`Minecraft.runGameLoop` 持有 `scheduledTasks` 的 monitor 时调用 `Util.runTask`；`addScheduledTask` 则需要进入同一个 monitor。

旧路径可能形成：

1. Netty 持有 Prediction 的 mutex，回放包进入 `PacketThreadUtil`，等待 `scheduledTasks`。
2. 主线程持有 `scheduledTasks`，处理击退包触发 `AppliedVelocityEvent`，等待 Prediction mutex。

递归 mutex 只能处理同线程重入，无法解决这一跨线程循环等待。混用 Netty 排队回放和主线程立即回放，还可能让后续包越过已排队的前序包。

## 生命周期与验证

任务使用原版 Java 类；DLL 卸载后已排队任务仍可正常运行。恢复钩子并等待回调退出后，再清理剩余任务标记的 JNI 全局引用。入站调度失败时释放标记，保留 Java 异常。

运行 `Client/test/network/lunar-incoming/run.ps1`：检查本机 Lunar 的真实 mapping，编译实际 C++ 收包钩子的并发测试，检查取消与回放顺序、错误与卸载清理，并在独立 Java 17 JVM 的 `-Xverify:all` 下验证真实 Runnable 改写后的两个分支。测试不注入正在运行的游戏，不创建 Java 源文件。

2026-09-20 00:56 的 AppHangB1 没有线程转储，因此上述死锁是代码与回归测试验证的缺陷，不能仅凭日志断言它是那次卡死的唯一原因。
