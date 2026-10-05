# InvMove / Rise modes

原生 C++ 实现，参考 `skid/Rise-6.9.5-main/src/main/java/com/alan/clients/module/impl/movement/inventorymove/bypass/WatchdogBypass.java`。

- Movement → InvMove，包含 Normal、Buffer Abuse、Cancel、Grim、Grim 2、Watchdog，默认 Normal，与 Rise 一致。旧配置没有保存 Mode 时使用 Normal；需要原行为请选择 Watchdog。
- Lunar 1.8.9 下 InvManager 启用且关闭 Inventory Only 时自动借用当前选择的模式，无需打开独立 InvMove 开关。装备、换槽、丢弃统一经过点击入口；只有 Watchdog 首次操作等待输入停顿完成。Refill 优先级保留。其他版本保持原整理逻辑。
- Normal 恢复界面移动；Cancel 取消 C16 开背包状态、C0B OPEN_INVENTORY 和 C0D 关窗；Grim 与 Grim 2 在背包/箱子禁跑。
- Grim 独有 Manager Extra Sprint Ticks（默认 9，范围 0–20），按成功整理点击数设置 max(剩余计时, 本 tick 点击数 + Extra)；Grim 2 没有额外计时。
- Buffer Abuse：Clicks 默认 3（2–10），Amount 默认 5（1–10）。累计 Clicks 个 C0E 后开始缓存，第一个 PreMotion 等待，第二个发送 Amount 个无参 C0E，然后 FIFO 重放。等待期间只取消移动加速和跳跃，保留已有惯性。
- 专属参数只在对应模式显示；Cancel、Buffer Abuse 在没有 GUI 时也采样移动绑定。
- Prediction Mode 默认关闭；Ticks 为 1–20，默认 1；Measure Chest Open 默认开启。由现有配置、按键、模块管理器管理。
- 目前注册到 Lunar 1.8.9，其他版本未补齐这些发包/药水绑定，不声明已经支持。
- 排除聊天、Nova 的输入捕获和非游戏前台；采样当前实际按键绑定，包含方向与跳跃。
- Watchdog 包事件记录普通物品点击、开箱/NPC 交互尝试及 C03 间的容器操作标记；其他模式的包处理独立。
- 物品点击状态持续至离开背包；60ms 延迟不是每次点击重置。开箱测量在进入世界 50 tick 后运行，等待超时 40 tick；确认后每 5 tick 放行输入。
- 固定速度与 Prediction 两条分支，疾跑仲裁复用 Sprint；多方向输入通过统一 RotationManager 提交 70 度方向辅助与移动修复，速度向量使用原版 90 度方向计算。
- 断开、重生、世界/玩家替换、关闭模块清理状态。Buffer Abuse 缓存使用 JNI 全局引用和互斥保护；重放走 SendPacketNoEvent，不重新计数。模式切换/关闭释放当前队列，换世界丢弃队列；所有出口释放引用。

## 与参考代码的适配差异

- Rise 的 `cqL` 已从 ScriptEntity.getGroundTicks 和附带 JAR 的 Entity.onUpdate 确认是连续着地计数。Nova 使用本地计数，启用瞬间没有 Rise 的历史计数。
- 新增 Entity.moveFlying / EntityLivingBase.jump 原生入口守卫，实现 Buffer Abuse 取消；新增 moveEntityWithHeading 内 moveEntity 调用重定向，实现 Watchdog 空中 x/z 位移清零后执行原本的碰撞处理。非本地玩家不受限制。
- 本项目没有 Rise Speed 模块，未创建无效的 Speed 启停联动；参考中的该分支仅在 Speed 开启时触发。
- 额外增加窗口焦点/浏览器输入保护，以及跨世界状态清理，避免界面输入穿透和旧测量状态残留。
- Rise Buffer Abuse 未提供同样的禁用清理；Nova 主动释放避免队列跨模式残留。InvManager 无需开启 InvMove 即可借用模式，也是用户指定的适配。
- 事件采样阶段与统一 RotationManager 不同于 Rise 自身环境，不将算法移植或编译通过称作服务器行为 100% 等价。

## 验证

`Client/test/invmove/run.cmd` 验证 Buffer Abuse 周期/重置、Grim 计时/延长，以及 Watchdog 点击延迟边界、重复点击、离开界面、开箱延迟、五 tick 放行、Prediction 和超时。

`Client/test/invmove/hooks.cmd <classes目录>` 使用生产规则对真实 Entity/EntityLivingBase 类应用三个新增钩子，检查指令、分支、StackMapTable 边界，不生成 Java 源码或测试桥接类。
DLL 编译不等于服务器验证；用户自行注入测试移动、打开箱子/背包、普通点击/Shift 点击、药水、跳跃与 Sprint 共存。
