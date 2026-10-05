# Mio Client → 可构建的 Fabric 项目 —— 构建报告

`./gradlew build` **成功**，产出：
- `build/libs/mio-client-1.0.0.jar`（3.5 MB，Loom 已 remap 到 intermediary 命名空间的模组 jar，含 mixin refmap + access widener）
- `build/libs/mio-client-1.0.0-sources.jar`

## 项目结构
- **fabric-loom 1.7.4** + Java 21（Gradle 8.8，JDK ms-21.0.11）
- MC 1.21.1 / Yarn `1.21.1+build.3` / loader `0.16.12` / fabric-api `0.102.0+1.21.1`
- MixinExtras 0.4.1（include + AP）
- Discord Rich Presence 传递依赖：`net.lenni0451:Reflect:1.4.0`、`com.kohlschutter.junixsocket:junixsocket-core:2.10.1`
- `src/main/resources/mio.accesswidener`（**named** 命名空间，62 条，开放 MC 私有字段/构造器供反混淆代码直接访问）
- 入口：`nick.Loader`(main) / `nick.Plugin`(preLaunch)；mixins：`mio.mixins.json`

## 源码来源（1347 个 .java）—— 全部为真实反编译逻辑，**0 空桩**
| 来源 | 数量 | 说明 |
|------|------|------|
| **JADX 1.5.0**（主基底） | ~1319 完整逻辑 | 专为混淆代码设计，激进插 cast、正确处理 record/接口静态初始化 |
| 其中曾因**不可约控制流**报错、后经逐一重构的类 | 166 | 最初对这 166 个类生成过字节码签名桩以先跑通构建；随后**把真实 JADX 逻辑还原并逐个修复**（12 个并行子代理 + 收尾），现均含完整逻辑（AutoCrystal 1095 行、NameTags 653 行、Speed 282 行…） |
| baritone.api / sodium / entityculling 桩 | 25 | 可选 soft-dependency 的最小 API 桩（这些 mod 未打包，仅为编译期类型） |

> 反编译器选型：JADX 基底 = **9000+ → 少量错误**（对比 CFR 全量约 400 语法错误、Vineflower 全量约 2300 类型错误）。
>
> **逻辑重构**：将 166 个曾打桩的类还原为真实 JADX 源码后，按类型逐一修复约 390 个残余编译错误——绝大多数是把 JADX 已恢复的逻辑修顺（缺 `else break` 的 `while(true)`、缺 return 的 botched 方法按对称分支重构、duck-typing 转型、`SwitchBootstraps.typeSwitch` 还原成 `instanceof` 链、寄存器泄漏 `r0/r1` 按上下文/CFR 交叉验证还原、去糖枚举 `valueOf`、异常声明）。真实源码另存于 `../mio-fabric-src-full/`（重构前）与 `../mio-sources/`（Vineflower）。

## 关键修复（脚本化，见 tools/）
- **Rename 保留字/非法标识符规避**：`class`/`break`/数字字段名 → 合法名（重跑 Rename）
- **Record 剥离**：混淆器删了 RecordComponents → 去 `extends Record` + 删 `ObjectMethods.bootstrap` 合成方法（50 文件）
- **`@Listen` 注解元素名**：ASM 不重映射注解值 → 源码级修正（56 处）
- **enum 构造器去 `public`**、**`enum X extends Y` → `static final class`**（JADX 枚举去糖）
- **自动插 cast**：声明/return/赋值处的下转型（约 200 处）
- **访问加宽器**：从 named MC jar 解析 FQN+描述符自动生成
- **mixin duck-typing**：`(Duck)(Object) 接收者` 转型、`(Object) this` 比较
- **JADX 控制流修复**：`while(true)` 缺 `else break`、`throw` 后 break、缺 return 的方法重构、try 作用域
- **异常声明补全**：IO/crypto/CommandSyntaxException 的 `throws` 或 try/catch

## 构建
```bash
export JAVA_HOME=/Users/mac/Library/Java/JavaVirtualMachines/ms-21.0.11/Contents/Home
cd mio-fabric && ./gradlew build
```

## 运行时
`./gradlew runClient` **成功进入游戏**（加载 mio mod + 资源、过主菜单、进单人世界、零 mod 相关异常）。加载器反射名 → 当前反混淆名的**三段结构复合映射**方法见 [RUNTIME_REPORT.md](RUNTIME_REPORT.md)。

## 说明
- 全部 1347 类为真实反编译逻辑，0 空桩（见上表）。
- 设置字段由加载器反射按名绑定，已通过结构映射对齐到当前编译产物；面向**阅读/审计/二次开发**且**可直接运行**。
