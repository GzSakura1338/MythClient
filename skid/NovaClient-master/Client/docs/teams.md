# Teams

入口：Misc → General → Teams，齿轮内只有 Mode，默认 Color。开启 Teams 后统一过滤队友；关闭后 AimAssist、KillAura 和 AutoBlock 都不进行队友过滤。三个模块不再保留独立的 Ignore teammates 开关。

- **Color**：比较有效队伍颜色。不同队伍同色仍视为队友，与 Starain 的颜色比较语义一致；无颜色不回退为白色，但显式白色仍是有效颜色。
- **Scoreboard**：通过 UUID 从玩家列表取得队伍，比较非空的队伍注册名。双方无队伍、玩家信息尚未到达或读取失败，均不判为队友。
- **Armor**：移植 Starain 1.20.1 的护甲判定。双方头盔、胸甲都必须是显式染色的皮革装备，头盔颜色对应相同且胸甲颜色对应相同才判为队友；不检查裤子和鞋子，也不依赖计分板。任意一件缺失、未染色、非皮革或读取失败均不匹配。头盔与胸甲自身可以不同色。模式追加在原有两个模式之后，旧配置索引保持不变。

移植参考 `skid/Starain-1.21.11-master/.../misc/Teams.java`。修正了原项目 `Objects.equals(null, null)` 的误判，并移除了 Nova 旧逻辑中对普通名字前两个字符的比较。

KillAura 在目标筛选、Hit select 和实际攻击前检查统一入口；AutoBlock 的目标选择和攻击回退也使用同一判断。Teams 不拦截用户的原版手动攻击，也不修改服务器队伍。

版本映射集中在各自的 `.inc` 文件的 `NOVA_TEAMS_MAPPING` 段：Lunar 1.8.9 使用 ScorePlayerTeam / EnumChatFormatting；Forge 1.20.1 使用 PlayerTeam / ChatFormatting；26.3 使用 PlayerTeam / Optional<TeamColor>。JNI 引用在卸载时释放，世界和队伍数据实时读取。

Armor 同样使用上述映射段：1.8.9 读取 `getCurrentArmor(3/2)`，通过 `ItemArmor.hasColor/getColor` 排除非皮革和未染色装备；1.20.1 使用 `EquipmentSlot.HEAD/CHEST` 和 `DyeableLeatherItem.hasCustomColor/getColor`；26.3 使用对应装备槽、皮革物品身份和显式 `DYED_COLOR` 组件，不使用默认颜色回退。全部由 C++ JNI 实现，不生成或注入 Java 源码。

旧配置中的 Ignore teammates 字段不再读取。旧配置没有 Teams 项时，模块默认关闭，需要手动开启后保存配置。

验证入口：`Client/test/misc/teams/run.ps1`、`Client/test/combat/killaura/run_killaura_cps.ps1`、`Client/test/combat/autoblock/run_autoblock.ps1`。
