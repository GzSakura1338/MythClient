# Mio Client → 运行时可用（runClient 进入游戏）

`./gradlew runClient` **成功启动并进入游戏**：
- 加载 `mio` mod 及其全部资源（`Reloading ResourceManager: … mio`）
- `Sound engine started` / `OpenAL initialized`
- 通过主菜单（TitleScreen）→ 加载进单人世界（`Player731 joined the game`）
- 全程**零 mio/加载器相关异常**（仅剩 fabrishot/lithium/sodium 等未安装 soft-dep 的无害 mixin 警告）
- 进世界地形正常加载、clickgui 可用（见下「macOS 卡 LOADING TERRAIN」）

## macOS 专有致命 bug：进世界卡在 LOADING TERRAIN / clickgui 打不开

**症状**：进世界后一直显示 LOADING TERRAIN、按 Right Shift 打不开 clickgui。二者**同根**。

**根因**：`MixinMain` 在 `Main.main` HEAD 强制 `System.setProperty("java.awt.headless","false")`（作者本意，Windows 无碍）。macOS 上 GLFW 必须 `-XstartOnFirstThread` 独占 Cocoa 主线程事件循环，而**非 headless 的 AWT**（Fonts 模块用 `java.awt.Graphics2D/BufferedImage` 渲染字体，经 `MixinMinecraft` 每帧 RenderEvent 在渲染线程触发）会争抢同一主线程事件循环 → **`glfwPollEvents` 原生死锁**（渲染线程 CPU 冻结、卡在 `org.lwjgl.glfw.GLFW.glfwPollEvents`）。`glfwPollEvents` 正是处理键盘事件之处，死锁 → 键盘回调不触发 → `DownloadingTerrainScreen`（`shouldClose=()->false`，靠 tick 里 30s 超时或事件关闭）永不 tick 关闭、且 Right Shift 的 `KeyEvent` 永不派发 → clickgui 打不开。

**定位**：全 mixin 关 → 地形正常（1450 区块、无 DownloadingTerrainScreen、渲染线程在正常 `glfwWaitEventsTimeout`）；全开 → 死锁。用 `--quickPlaySingleplayer` 做确定性复现 + `jmap -histo:live | grep DownloadingTerrainScreen` 判活，二分 154 个 mixin 定位到 **`MixinMain`** 单一元凶。

**修复**（`me/mioclient/mixin/MixinMain.java`）：macOS 强制 `headless=true`，其他平台保留作者的 `false`。headless 下 AWT 走离屏模式，Fonts 的 `BufferedImage` 字体光栅化照常工作，不再抢 Cocoa 事件循环。验证：全 154 mixin + 本修复 → 进世界地形正常加载、渲染线程健康。
```java
boolean mac = System.getProperty("os.name","").toLowerCase().contains("mac");
System.setProperty("java.awt.headless", mac ? "true" : "false");
```

## clickgui 打不开 / 崩溃 / 一直闪红（反混淆残留 bug）

修复 [macOS 死锁] 后 clickgui 可弹出，但暴露出三个反编译残留 bug：

1. **打开即崩 `IllegalAccessError`**（`SettingPredicates.create`）：用 `LambdaMetafactory` 把 Setting 的**私有**合成 lambda 包成 Predicate，生成的 lambda 类在调用点二次校验访问权限而失败。改为**直接绑定并调用 `MethodHandle`**（`insertArguments`+`invoke`，句柄在受信 lookup 处已完成访问检查，调用时不再校验）。

2. **整个 clickgui 一直闪红**（`ArrayListPresetHelper2`）：**字段遮蔽**——父类 `SearchHelper4_19.num`(Y 位置) 与子类 `ArrayListPresetHelper2.num`(闪烁计数器) 被 Rename 都命名 `num`，Java 遮蔽下子类里所有 `this.num` 都指向闪烁计数器；渲染里 `if(this.num>0)` 用了位置值(恒>0)→所有模块触发红色脉冲动画。把子类字段重命名 `flashNum`（消除遮蔽），闪烁引用改 `flashNum`，位置引用 `this.num` 自然解析到父类。

3. **打开崩 `Double cannot be cast to Integer`**（`NumberSetting`）：损坏配置(旧版 bug 存的)把整型设置存成 Double，加载后 `setting.getValue().intValue()` 处 `(Integer)` 转型崩。根修：`NumberSetting` 里 override `do2323`，所有值赋值统一按 **min(object3，按声明类型稳定设置)** 的类型协调（含 `Setting.do2333` 里 `do2323(min/max)` 的钳制路径），`getNumber3025` 同样以 min 为类型基准。**注**：极端损坏的旧配置个别设置仍可能残留——如遇崩溃，删除 `run/mio-fabric/*.json`（旧版 bug 的产物）即可全新生成。

验证：进世界地形正常、Right Shift 打开 clickgui、紫色主题正确渲染、无红闪、无崩溃。

## clickgui 参数(子设置)显示问题

打开 clickgui 后展开模块，参数(滑块/数值)显示错乱，均为反编译的字段合并/遮蔽 + Java 语言细节 bug：

4. **数值滑块文本重叠在顶部/大片空白**：`SettingSearchHelper419` 有 `long num`(滚动计时器) 遮蔽父类 `SearchHelper4_19.int num`(Y位置); `StopwatchSettingSearchHelper419` 又有自己的 `int num/num2`(滑块条 X/Y)。子类 `this.num`(本应是位置) 解析到了错误字段 → 滑块全渲染在 Y=0 重叠。改名 `scrollTime` / `barX`/`barY` 消除遮蔽, 位置引用 `this.num` 自然解析到父类。

5. **滑块一闪一闪**：`getString95()`(光标闪烁) 每 500ms 切换 `flag2`, 但 `flag2` 也是编辑模式标志(决定显示"值"还是编辑缓冲)。obf 里本是光标(`nzYpu`)、编辑(`en7dw`)两个独立布尔被反编译合并。改用独立 `cursorBlink` 字段做闪烁。

6. **★ 总根源: 整型数值设置被存成 `Double` → `getValue().intValue()` 处 `(Integer)` 转型崩溃**(字体空白/崩溃、HUD 崩溃、clickgui 崩溃全由此起)。`NumberSetting.getNumber3025` 用三元表达式 `ref instanceof Double ? Double.valueOf(..) : ref instanceof Integer ? Integer.valueOf(..) : Float.valueOf(..) : Long.valueOf(..)` 做类型协调——**三元操作数为 Double/Integer/Float/Long 混合包装类型时, Java 二元数值提升会把结果统一拆箱提升到 `double` 再装箱成 `Double`, 无论走哪个分支都返回 Double**! 反编译把原本的 if-else 折叠成三元, 引入此 bug, 导致类型协调完全失效。改回 **if-else**(每个 `return` 独立类型, 无提升) 后, 数值设置正确保持声明类型, 所有 Double→Integer 崩溃消除、自定义字体(Fonts)正常渲染。另加 `NumberSetting.getValue()`/`do2323` override 以 min 类型兜底残留污染。

> 提示: 反编译常把 `if/else if` 链折叠成嵌套三元。当分支返回**不同的数值包装类型**时, 这是语义改变的 bug(数值提升), 必须还原成 if-else。

## 核心难题：加载器反射名 vs 反混淆源码名

加载器 `nick.*` 用**反射**按字符串名绑定被混淆类的类/字段/方法（数千条注册表项，原始为 `me.mioclient.m$$…` 混淆名）。反混淆后源码已重命名，且重命名**不确定**（HashMap 迭代序 → 类/方法名每次 Rename 不同；手工把去糖枚举转回真 enum 时又改了常量名）。因此加载器的反射名必须映射到**当前编译产物**的真实名，否则 `NoSuchField/NoSuchMethod/ClassNotFound`。

## 解法：三段结构复合映射 obf → yarn → current

`tools`/临时脚本链（`MatchAll3.java` + `remap4.py`）：

1. **obf → yarn**（`mio-cy.jar` → `mio-yarn.jar`）：按**类指纹**匹配（enum/interface 标志 + 方法/字段数 + 归一化描述符集合 + LDC 字符串常量集合），963 类唯一匹配、0 nomatch。yarn 反编译**保留声明顺序**，故类内成员按顺序配对可靠。
2. **yarn → current**（`mio-yarn.jar` → `build/classes` 编译产物）：同名类（FQN 一致）内按**精确描述符**配对——因两者同命名空间，`Setting`/`Stopwatch` 等真实类型名相同，可消歧同为 `L…;` 的多个引用字段（**关键**：归一化会把所有对象字段混成一桶导致错配）。这一步吸收手工改动（18 个枚举常量重命名 MIN/MAX/…、大小写、`_` 还原）。
3. **复合**：以全局唯一的 obf token `m$$…` 为键，输出 `obf → current`。兜底：未匹配的 604 歧义类走 `renamemap`（obf→yarn，这些多为无手工改动的工具类，yarn==current）。

`remap4.py` 用**嵌套感知**正则一次性重写加载器：
- 类引用（slash + dotted）：`me/mioclient/m$$X($m$$Y)*` 与 Class.forName 点式 `me.mioclient.m$$X$m$$Y`（嵌套用 `$m$$` 续接，**非** `.m$$`——早期 bug 致 `HoleESP$内层` 被当字段映射成乱名）。
- 方法名反射（`getDeclaredMethod("m$$…")`）走 method map。
- 其余 token 走 field map。
结果：`class=14238 field=11995 method=4 | 0 缺失`。

## 少量方法反射手工定名（顺序匹配对 lambda 不可靠）

lambda/合成方法名带编译器计数器（`lambda$new$2`/`lambda$do1$3`），obf/yarn/current 三方都不同、无稳定映射。加载器仅 5 处方法反射，按**语义 + 当前签名**定名：
| 位置 | 反射目标 | 当前真实名 |
|------|----------|-----------|
| `Modules`/`Commands` | ModuleManager 注册 `(Module)`/`(Feature)` | `register` |
| `Settings:92` | Module 加设置 `(Setting)` | `add` |
| `SettingPredicates:84` | Setting 注册谓词 `(Predicate)` | `do2343` |
| `SettingPredicates:45` | `findStatic` Setting 的谓词工厂 `(Setting[],Object)Z` | `lambda$getSetting2342$0` |

## 其余加载器修复（CFR 反编译瑕疵 / 运行时兜底）
- `MioClassLoader.read()`：缺失 `.bin` 返回 null（资源已解包到真实路径，见 [RESOURCES.md](RESOURCES.md)）。
- `SettingSubdata`：intermediary 名 `class_7923/class_2378` → `Registries/Registry`（+ `field_411xx` → BLOCK/ITEM/…）。
- 泛型转型：`Reflection` `(T[])(Object)`、`SettingPredicates` `(Predicate<?>)`、`SettingTypes` 去多余 `(Object)`。

## 复现
```bash
export JAVA_HOME=/Users/mac/Library/Java/JavaVirtualMachines/ms-21.0.11/Contents/Home
cd mio-fabric && ./gradlew runClient
```
> 加载器源码 = 结构映射后的 `nick/*.java`（已在 `src/main/java/nick/`）。若重新 Rename 生成新反混淆源码，需按上述 3 步重建映射再重映射加载器（`mio-cy.jar`=obf、`mio-yarn.jar`=deobf 参照、`build/classes`=当前产物）。
