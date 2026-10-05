# AutoBlock

Combat → General 中提供独立的 AutoBlock，点击齿轮配置。模式为 Lag、Vanilla、Grim；参数、默认值和按模式显示的规则，与 KillAura → AutoBlock 共用 `AutoBlockModuleBase` 定义。KillAura 原有配置路径保持不变，独立模块的参数另行保存。

两处共用一个 `AutoBlock` 引擎，由 `AutoBlockModule.cpp` 统一驱动 tick、发包观察和动画。独立模块开启时优先使用它的设置；关闭后，若 KillAura 的 AutoBlock 仍开启，恢复使用 KillAura 的设置。交接或切换模式会释放旧格挡、清理缓冲与待处理攻击。

独立模式无需开启 KillAura，也不负责自动瞄准或生成攻击。Vanilla／Grim 会在清理原版点击队列前保留手动点击或 AutoClicker 的攻击请求，再交给现有格挡状态机执行。Lag 保留原有右键触发行为。AutoBlockHUD 继续读取同一份格挡统计。

当前实际格挡能力与原实现一致，仅 Lunar 1.8.9 生效；其他版本保留模块和设置入口。

验证：`Client/test/combat/autoblock/run_autoblock.ps1` 检查原有状态机、独立点击输入、两处开关交接与清理；`run_killaura_cps.ps1` 检查 KillAura 攻击间隔回归。游戏内注入验证由用户完成。
