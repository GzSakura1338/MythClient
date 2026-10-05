# Nova Minecraft Proxy 系统设计

日期：2026-10-01。状态：首版原生实现、协议测试、连接钩子离线检查及页面预览已完成；真实 Minecraft 服务器连接尚待注入后验证。

## 1. 需求与交付目标

在 ClickGUI 左侧增加与 Modules、Client、Browser、Accounts 同级的 Proxy 大分类。用户手动配置 HTTP CONNECT 或 SOCKS5 代理地址、端口及可选认证；随后加入多人服务器时，Minecraft 登录、游戏和该连接上的后续通信经过指定代理。

首版采用一个活动代理配置，不引入代理池、自动轮换、测速排行榜或其他复杂参数。UI 沿用项目现有字体、输入框、下拉菜单、开关、按钮、模糊背景和页面动画。代理系统独立于 Minecraft 模块，使未来其他游戏可以复用协议、配置和状态接口。

所有新逻辑使用原生 C++ / WinSock / JNI。允许读取 Minecraft Java 作为参考，禁止编写 Java 源码、辅助 Java 类或新 Java 字节码。Minecraft 接入使用项目已经存在的原生钩子与方法改写基础设施。

### 1.1 流量范围

| 流量 | 首版行为 |
|---|---|
| 加入多人服务器的 TCP 连接 | 必须经过配置代理，包括 Minecraft 握手、登录、加密和游戏包 |
| 原连接内协议切换与压缩 | 完整转发，不改变原版处理 |
| 现代服务器 Transfer 后新连接 | 重新创建代理隧道，按新目标连接；保留原版 Transfer 状态 |
| 单人 IntegratedServer / LocalChannel | 原版本地路径，不建立代理 |
| 服务器列表状态查询、旧版兼容 ping | 首版不接管；页面明确“仅加入服务器连接” |
| 微软/Xbox/Minecraft 会话认证 HTTP、皮肤、资源下载 | 不接管；它们与游戏 TCP 是不同连接 |
| 浏览器、SMTC、Lunar 自身服务 | 不接管 |
| DNS 和 SRV | 首版保留原版解析；远程 DNS 单独列为后续能力，不宣称全流量代理 |

启用代理后，该范围内的连接失败必须失败关闭，不能自动直连。禁用只影响下一次新连接，已建立的隧道持续到断开或熔断。

## 2. Recaf MCP 分析证据

以下结论来自本轮 Recaf MCP `open_workspace` 与 `get_class_source`，并与 Nova 本地代码交叉核对。使用反编译源码梳理行为；最终实施仍需字节码调用点和运行时 Mixin 后的匹配审计。反编译器出现的无效局部变量命名不是可直接复制的源代码。

### 2.1 已分析的文件

| 客户端 | 本机资源 |
|---|---|
| Lunar 1.8.9 | `C:/Users/13366/.lunarclient/offline/multiver/cache/72c13d03/d3098b28/bake.zip` |
| Lunar 1.7.10 | `C:/Users/13366/.lunarclient/offline/multiver/cache/10d9b7f1/aed3ca70/bake.zip` |
| Lunar 1.21.11 | `C:/Users/13366/.lunarclient/offline/multiver/cache/d1937f9/76a6f84/bake.zip` |
| Forge 1.20.1 | `C:/Users/13366/AppData/Roaming/.minecraft/libraries/net/minecraftforge/forge/1.20.1-47.4.23/forge-1.20.1-47.4.23-client.jar` |
| NeoForge 1.21.8，供 EaseCation 基础 ABI 核对 | `E:/MCLDownload/Game/.minecraft/libraries/net/neoforged/neoforge/21.8.52/neoforge-21.8.52-client.jar` |
| Minecraft 26.3（实施阶段补查） | `C:/Users/13366/AppData/Roaming/.minecraft/versions/26.3/26.3.jar` |

设计阶段 Minecraft 26.3 只阅读已有映射；实施阶段在 `C:/Users/13366/AppData/Roaming/.minecraft/versions/26.3/26.3.jar` 找到实际 JAR，已通过 Recaf 补查。NeoForge 基础 JAR 不等于 EaseCation 运行时全部 Mixin，需要运行时复核。

### 2.2 Lunar 1.8.9

已读取 `GuiConnecting`、`NetworkManager`、`ServerAddress`。

1. `GuiConnecting` 从 ServerData 解析 ServerAddress，开启 `Server Connector #N` 线程连接。
2. Lunar 的 Mixin 将地址解析挪到 `redirect$...$getIpString`，使用 Lunar 自身地址解析服务返回 `InetSocketAddress.getAddress()`。不能只拦截原版 `InetAddress.getByName`，那个调用已被替换。
3. `redirect$...$connectEvent$v1_8` 调用 `NetworkManager.createNetworkManagerAndConnect(InetAddress,int,boolean)`；观察到该 Lunar 路径实际传 `false` 作为 native transport 参数。
4. 上述工厂创建原版 Netty Bootstrap、ChannelInitializer、NioSocketChannel 或 EpollSocketChannel，并执行 `Bootstrap.connect(InetAddress,int).syncUninterruptibly()`。
5. 通道管线为 `timeout → splitter → decoder → prepender → encoder → packet_handler`。随后原版可添加加密与压缩处理器。
6. 工厂返回后立即设置 NetHandlerLoginClient，发送 `C00Handshake(47,host,port,LOGIN)` 和 `C00PacketLoginStart(profile)`。
7. 本地世界走 `provideLocalClient(SocketAddress)` 和 LocalChannel，必须排除。
8. `ServerAddress.fromString` 支持 IDN、括号 IPv6，并对默认端口执行 `_minecraft._tcp` SRV 查询；Lunar 后续解析还有自己的实现。

确定的 transport 接入候选：该工厂内的 `Bootstrap.connect(InetAddress,int)`。不要以 Mixin 随机后缀识别高层连接方法。

### 2.3 Lunar 1.7.10

已读取 GuiConnecting 和 NetworkManager。

- 工厂名称为 `NetworkManager.provideLanClient(InetAddress,int)`，并非 1.8.9 名称；这里的名称不代表只供 LAN 使用，GuiConnecting 加入服务器也调用它。
- 工厂同样使用 Bootstrap、NioSocketChannel、`connect(InetAddress,int).syncUninterruptibly()`。
- Lunar 在 Bootstrap.handler 调用处还包了一层自己的 `wrapHandler`，应保留该初始化器。
- 登录路径发送协议版本 5 的 C00Handshake 和原版登录开始包。
- LocalChannel 连接另有路径，不代理。

因此 1.7 必须用独立映射，不能猜测调用 1.8 工厂。

### 2.4 Lunar 1.21.11

已读取 `class_412`（ConnectScreen）、`class_2535`（Connection）、`class_6370`（ServerNameResolver）。

- ConnectScreen 的 `method_2130` 开启连接线程；`class_6370.field_33745.method_36907(ServerAddress)` 完成目标解析，得到已解析 InetSocketAddress。
- ServerNameResolver 先执行普通解析和地址允许检查，再查询 SRV redirect，存在 redirect 时再次解析并过滤地址。必须保留原版地址拦截策略。
- `Connection.method_52271(InetSocketAddress, class_12239, Connection)` 创建 Bootstrap，选择原版 transport，再调用 `connect(address.getAddress(), address.getPort())`。
- 原版等待 ChannelFuture `syncUninterruptibly` 后调用 `method_56326` 发起登录协议，再发送 `class_2915` 登录开始包。
- ChannelInitializer 添加 30 秒读超时，通过 `method_48311` 装配分包、流量控制、协议配置、编码器，再调用 `method_53859` 添加 `hackfix` 与 packet_handler。
- 原版 ConnectScreen 取消按钮先置取消标记，再取消 ChannelFuture、关闭 Connection。现代连接还携带 TransferState。
- 本地世界是 `method_10769(SocketAddress)`，不走网络代理。

注意：`class_12239` 是这个版本的 transport 参数类型，不能沿用其他现代版本的 boolean。

### 2.5 Forge 1.20.1

已读取 ConnectScreen 和 Connection。

- ConnectScreen 中连接线程通过 `ServerNameResolver.f_171881_.m_171890_` 解析目标。
- `Connection.m_290025_(InetSocketAddress,boolean,Connection)` 返回 ChannelFuture，内部 Bootstrap 最终调用 `connect(address.getAddress(),address.getPort())`。
- ConnectScreen 等待该 Future 后继续登录。方便工厂 `m_178300_` 也封装了该连接方法。
- 本地世界工厂 `m_129493_(SocketAddress)` 使用 LocalChannel 和 Forge 自己的 login channel 注册，必须原样保留。

### 2.6 NeoForge 1.21.8 / EaseCation 适配基础

已读取基础 JAR 中 ConnectScreen 和 Connection。

- `Connection.connect(InetSocketAddress,boolean,Connection)` 返回 ChannelFuture，内部最终也调用 `Bootstrap.connect(InetAddress,int)`。
- ConnectScreen 先调用 `ServerNameResolver.DEFAULT.resolveAddress`，等待连接 Future 后执行 `initiateServerboundPlayConnection` 和原版登录开始。
- 新版本加入 TransferState，但仍需保留原版取消和状态跳转。

### 2.7 从证据得到的设计限制

实施阶段补查 26.3：ConnectScreen$1.run 同样先解析地址，再调用 `Connection.connect(InetSocketAddress,EventLoopGroupHolder,Connection)`；工厂内部最终调用 Bootstrap.connect(InetAddress,int)。取消字段为 ConnectScreen.aborted，线程通过 this$0 持有父screen。LocalChannel保持独立。已按该实际描述符接入，不能使用1.21.8的boolean transport描述符。

1. 所有已查版本都在 Netty 创建实际连接前拿到了 IP，单独替换 TCP 终点无法自动实现代理端 DNS。
2. 多人登录紧跟连接成功，不能让代理握手响应进入 Minecraft splitter，也不能先向代理发 Minecraft 握手。
3. 高频网络包、编码、加密、压缩都在原版 Netty 中处理，代理不应接入 AntiKB/AutoBlock 的包延迟队列。
4. 高层 UI、Mixin 后缀、transport ABI 有版本差异；最终 transport 连接操作比名字混淆的 UI 方法稳定。
5. 不应使用全进程 `ProxySelector`、JVM 系统属性或全局 WinSock connect detour，避免意外代理浏览器、微软授权和 Lunar 后台服务。

## 3. 总体架构决策

采用**原生单连接本地 TCP 中继**，保留原版 Netty 网络栈。

```text
ClickGUI Proxy 配置 → 不可变 ConfigSnapshot
                               ↓
多人连接线程：原版域名/SRV解析 → 目标 TargetEndpoint
                               ↓
                 Nova PrepareTunnel（C++ 工作线程）
                               ↓
                 连接代理 → 认证 → CONNECT成功
                               ↓
                 绑定 127.0.0.1 临时监听端口
                               ↓
原版 Bootstrap.connect → 仅本次 transport 参数替换为 loopback
                               ↓
Minecraft Netty ↔ 本地中继 ↔ 已建立的代理隧道 ↔ 目标服务器
```

原版连接结构和握手中的 host/port 保留原目标，只替换最后一次 transport connect 的参数。不能直接修改 ConnectScreen 使用的整个 InetSocketAddress，否则现代版随后 `getHostName/getPort` 可能把 localhost 发进 Minecraft 握手，破坏虚拟主机路由。

### 3.1 选择理由与代价

- C++ 自行完成 HTTP/SOCKS5 握手，无需新增 Netty proxy 类，也不依赖游戏是否携带 `netty-handler-proxy`。
- 不编写 Java ChannelHandler，避免违反项目语言约束和 Netty 版本兼容问题。
- 握手完成后才让原版连接到中继，代理响应不会进入 Minecraft 协议处理器，登录开始时序清晰。
- 不依赖操作系统全局代理或电脑上已有浏览器。
- 代价是额外一段 loopback 连接、内存复制、socket 和原生工作线程。不能承诺零延迟；验收以流量压力测试和测量数据为准。
- 连接持续依赖 DLL 的中继代码。熔断必须关闭该代理游戏连接，不能在卸载后继续保留它，也不能无感切为直连。

## 4. 连接拦截和原版协议保真

### 4.1 精确作用域

原生回调替换已确认的多人网络工厂内部 `Bootstrap.connect(InetAddress,int)`，保持返回值 `ChannelFuture` 不变。使用现有 ClassFileEditor 重定向调用的能力；回调签名接收 Bootstrap receiver、原地址、端口，并返回原始 Netty Future。

这属于现有项目的原生 JVM 方法调用改写，不创建新的 Java 辅助实现。各版本规则必须在实际 class 字节上审计，安装计数必须准确。

同时从 ConnectScreen / GuiConnecting 的本次加入过程建立 JoinAttempt 标记，明确连接属于玩家加入，而非服务器列表 ping。只有标记、版本和目标匹配的连接才走代理。

不能仅用线程名 `Server Connector #N` 判断，也不能仅依据端口 25565。标记使用 C++ Win32 TLS/FLS 或显式对象关联，不能引入反射 DLL 不支持的静态 TLS 存储。整个加入连接完成、取消和异常出口均要清理。

如果代理启用而已识别为加入的连接缺少必要元数据或版本能力，不允许调用原版直连；返回明确失败。未属于首版作用域的列表 ping 和 LocalChannel 仍使用原版路径。

### 4.2 JoinAttempt 和 Tunnel

每次加入保存：

- attemptId、不可变配置 revision、客户端 profile。
- requestedHost/requestedPort：输入或服务器页面的原始逻辑目标，供展示。
- resolvedEndpoint：原版解析/SRV 后真正要连接的 IP 和端口。
- protocolHandshakeIdentity：原版登录握手使用的地址身份，保持原版值，不自行重写。
- shared CancelState：取消标志、停止事件、阶段 deadline。
- tunnelId、loopbackEndpoint、连接 Future 的必要临时引用。

接入时只改 transport，不改 Minecraft 握手内容；SRV 到不同物理主机时仍保留原版自身的握手行为。逻辑地址、物理目标、loopback 三者不得混用。

### 4.3 线程约束

- UI/渲染线程只修改配置和读取状态快照，不做 DNS、TCP connect 或阻塞 JNI 等待。
- 原版服务器连接线程可等待代理准备，但使用可取消 native event；不得在 Netty event loop 等待自己的连接 Future。
- 本地中继 worker 仅使用 WinSock 和 native 数据，不调用 Minecraft JNI，也不保存 JNIEnv。
- 在原版回调线程调用 Bootstrap.connect 并返回原始 Future；全局引用仅在确有跨回调需求时保留，结束必释放。
- status 使用轻量 mutex 或原子快照发布；绝不持有 UI/config 锁做 I/O 或调用 JNI。
- 不在 DLL 卸载后保留任何回调、线程、定时器或未完成 I/O。

### 4.4 握手前等待与取消

PrepareTunnel 应把代理 TCP 和协议握手交给 worker。原版连接线程等待 readiness/cancel，避免把慢网络操作塞进 render 或 Netty 线程。

原版 ChannelFuture 在 Bootstrap.connect 返回前尚不存在，因此**只保留原版 Future.cancel 不够**。必须接入 ConnectScreen 原版取消动作，让它同时触发 JoinAttempt 的取消事件；取消窗口回调只设置标志和关闭可取消 socket，不能等待 worker join。

高层 screen 被销毁、开始另一连接、断开、卸载均取消旧 attempt。随后原连接线程退出并交给原版失败/取消逻辑。应使用小型异常原因或原版断开接口，禁止写 UI 对象字段模拟任意状态。

Loopback 建立失败时关闭已建立的上游代理，原 Future 的错误保持可被原版屏幕捕获。

## 5. 原生中继实现

每个 Tunnel 独立监听 `127.0.0.1:0`，使用系统分配端口，监听 socket 不对公网绑定，设置独占地址策略。上游隧道准备成功后开放本次监听；最多接受一个 Minecraft 下游，再立即关闭监听。

仅绑定 loopback 不构成身份认证。实现时校验接受连接的 TCP 所有者 PID、tuple 和 attempt 对应关系（Windows TCP 表），确定属于本游戏进程后才转发。错误下游连接不得消费唯一机会或读取上游数据。若所有者校验不可用，失败关闭，不能将公网或任意本机进程当作可信。

设置本地接入期限，避免原版连接取消后留下监听端口。上游远端提前发出的应用字节可以留在内核接收缓冲；不要握手后无限提前读取。

### 5.1 双向流转

- 推荐第一版每个 tunnel 用一个 worker，非阻塞 socket + WSAPoll/事件驱动双向泵；后续有多连接规模需求再改 IOCP。
- 每方向初始 32 KiB 缓冲，最多 256 KiB；达到上限停止对应方向 recv，待对端发送腾出空间后继续。
- 处理 partial send、WSAEWOULDBLOCK、recv EOF、RST 和双向半关闭。
- EOF 后先发送已缓冲数据，再向另一侧 `shutdown(SD_SEND)`；另一方向仍可正常排空。
- 原版加密后只是字节流，中继不解析、不解密、不改包、不重排。
- 禁止用一个超大包作为假定边界；TCP 分段不等于 Minecraft 数据包边界。
- 游戏持续连接不设置额外短空闲超时，保留原版 keepalive/read timeout；设置关闭排空上限，例如 2 秒。
- 取消时唤醒 worker、关闭监听及 socket，再确认 worker 退出，最后释放 Tunnel 对象。

## 6. HTTP CONNECT

HTTP 指客户端到代理使用明文 HTTP/1.1 CONNECT，不代表 HTTPS 代理端 TLS。UI 使用“HTTP (CONNECT)”标签，避免用户误以为必须填写完整网页 URL。

请求格式：

```text
CONNECT target.example:25565 HTTP/1.1\r\n
Host: target.example:25565\r\n
Proxy-Authorization: Basic <base64(username:password)>\r\n
\r\n
```

无认证时不发 Authorization。IPv6 authority 必须为 `[IPv6]:port`，域名 IDN 转 ASCII。所有 endpoint、认证字符串禁止 CR/LF，不能让输入成为 HTTP header 注入。username 的 Basic 语义不能包含冒号。

解析要求：

- 增量读取直到 `\r\n\r\n`，允许分段响应；头部上限 32 KiB。
- 校验 HTTP/1.0 或 HTTP/1.1 状态行；2xx 表示隧道成功，407 显示认证失败，403 显示目标/端口被拒绝，其他状态显示安全的简短错误。
- 有限处理 1xx 临时响应，最大 4 次，避免无限循环。
- 不自动跟随 3xx 重定向，不尝试将 Minecraft 流量作为普通 HTTP 请求发送。
- 成功响应头后已经收到的字节属于目标流量，保留并作为下游首批数据转发，绝不丢弃。
- 非成功响应体可以丢弃并关闭，不把响应页面或凭据内容直接显示/写日志。
- 认证仅首版 Basic；Digest、NTLM、Kerberos 和 HTTPS proxy 列为不支持并明确提示。

## 7. SOCKS5

1. 发送 `VER=05, NMETHODS, METHODS`。不开认证只提供 00；开启认证只提供 02，避免配置了认证却被静默降级为无认证。
2. 校验代理返回 `VER=05`，方法必须是本次提出的方法；FF 表示不支持认证方式。
3. 02 认证使用 RFC1929，版本 01，username/password 各为 1 字节长度加字节序列，UTF-8 长度为 1–255。非 ASCII 凭据可能与某些代理的实现不兼容，错误时明确提示。
4. CONNECT 请求：`05 01 00 ATYP DST.ADDR DST.PORT`。ATYP 为 IPv4=01、域名=03、IPv6=04；端口为网络字节序。
5. 首版使用原版解析后的 IP；未来 Remote DNS 分支使用 ASCII 域名，长度 1–255，不包含 IPv6 括号。
6. 校验 VER、RSV、REP，按 ATYP 读取完整绑定地址及端口，即使客户端不需要该地址也必须消费完。
7. REP=00 之后切换透明流转；其他 REP 映射为代理拒绝、网络不可达、主机不可达、拒绝连接、超时、不支持命令或地址类型。

无 UDP ASSOCIATE、无 BIND、无 SOCKS4。不能用 OS 全局 SOCKS 开关替代协议实现。

## 8. DNS、SRV、IPv6 和本地目标

### 8.1 首版明确边界

保持 Minecraft/Lunar 原版目标解析和 SRV 逻辑，目标 IP 通过代理建立 CONNECT。代理服务器自身的域名也需由本机解析，否则无法连到代理。

这保证游戏 TCP 经过代理，但 DNS/SRV 查询仍可能发生在本机。不能在页面和文档声称“隐藏所有目标域名”或“完全无 DNS 泄漏”。不提供实际上无效的 Remote DNS 开关。

### 8.2 后续 Remote DNS 设计

需要独立拦截原版高层解析链，保留原始域名，并对 ServerNameResolver/SRV/address-allowed 策略进行版本适配；仅 transport hook 做不到。

远程 A/AAAA 与 SRV 是两件事。标准 SOCKS5/HTTP CONNECT 都没有 SRV 查询功能：

- 指定端口：可以直接让代理解析域名。
- SRV：可选择本地 SRV 查询后将返回主机交给代理做 A/AAAA；页面应说明 SRV 仍为本地查询。
- 严格不查询本地目标 DNS：需要禁用 SRV 或新增明确的远程 DNS 服务设计，不能假装代理自动完成 SRV。

现代工厂会调用 getAddress，未解析 InetSocketAddress 不能直接塞回原版。Remote DNS 应把域名保存在 attempt 元数据，transport 使用真正的 loopback 地址；不得以未知域名失败后自动直连。

### 8.3 本地/LAN目标

IntegratedServer 的 LocalChannel 原样运行。用户手动加入 `localhost`、`127.0.0.1` 或私网地址仍是多人 TCP：代理启用时按字面目标代理，没有隐式直连例外。代理中的 loopback 指代理机器本身，UI 对这种目标显示一次解释，避免误以为是本机单人世界。

## 9. 状态机和失败处理

```text
Disabled / Ready
  → ResolvingTarget（原版）
  → ResolvingProxy
  → ConnectingProxy
  → Authenticating
  → ConnectingTarget
  → TunnelReady
  → ConnectingLocal
  → Active
  → Closing → Closed
任意阶段 → Cancelled / Failed → Closing → Closed
```

Disabled/Ready 是配置状态；连接状态属于具体 attempt，不能用一个布尔值代表所有生命周期。

默认 deadline：代理解析 5 秒，代理 TCP 8 秒，协议认证/CONNECT 8 秒，准备总预算 15 秒，本地接入 3 秒。各阶段共享总预算，不把阶段上限累加成无限等待。第一版使用内部常量，避免页面出现大量参数。

取消需及时唤醒，不靠每 10 秒等待一次来响应。代理解析使用可取消 resolver 或有界服务任务；禁止无法回收的 detached DNS thread。

配置快照变化不撤销现有连接；用户修改后显示“下次连接生效”。切换配置文件也遵循相同规则。主机重定向和重连重新生成 attemptId，不复用旧认证状态。

失败原因枚举应至少包括：配置错误、代理解析失败、TCP超时、认证失败、代理拒绝目标、地址类型不支持、响应格式错误、响应过长、本地连接失败、用户取消、系统卸载、连接中断。

错误反馈走原版连接失败界面和 Proxy 页面状态。通知只显示一次；不写包含密码或完整认证报文的错误内容。

## 10. ClickGUI 页面

现有 `PageLayout.h` 使用类别索引及特殊页 `ClientPage=-1`、`BrowserPage=-2`、`AccountsPage=-3`。Proxy 可新增独立特殊页 `ProxyPage=-4`，避免伪装成 Combat 等模块类别。

布局：

- 左侧大分类 Proxy，使用现有图标字体中的网络/连接图标；没有合适 glyph 时增加项目原生矢量图标，不用 emoji。
- 主面板顶部名称 Proxy 和 Enabled 开关。
- 连接区：Protocol 下拉、Host 输入、Port 数值输入。
- 认证区：Authentication 开关；启用后展示 Username、Password。Password 默认遮罩，可用小图标临时显示，不在普通状态栏回显。
- 操作区：Apply、Test Connection、测试中 Cancel；与模块页按钮反馈、焦点边框、输入动画一致。
- 状态区：简短结果，如 Ready、Connecting、Authentication failed；详细原因在现有风格的小弹窗中展示。

Host 只填主机/IP，不要求填写 `http://` 或 `socks5://`；粘贴带 scheme 时可以确定性拆分并提示协议，但不解析任意 URL path、query 或内联凭据。Port 为 1–65535 整数，无小数。

草稿修改在 Apply 后原子发布。Enabled 使用最后已验证配置；开启时遇到无效草稿不能切成“代理已启用但实际上直连”。无效配置保持 Enabled 请求状态并阻止新加入，给出修正提示。

最小必要说明只有“仅加入服务器；列表测速、登录授权和 DNS 不代理”，不堆砌技术提示文字。认证不可用、当前版本不可用时显示具体原因，不能用正常绿色 Ready 状态掩盖。

### 10.1 Test Connection

测试必须验证完整的代理认证和目标 CONNECT，不能只测代理端口通不通。测试区提供临时 Target Host/Port，默认取当前选择的 Minecraft 服务器，无法取得时需要填写；未经用户操作不自动连接任意外站。

测试建立隧道成功后立即关闭，不发送 Minecraft 登录，不创建账号会话，不依赖 HTTP 网站支持。显示“隧道建立耗时”，不能误称游戏 ping 或延迟测量。

## 11. 配置、凭据和模块间关系

建议 JSON schema：

```json
{
  "proxy": {
    "schema": 1,
    "enabled": false,
    "protocol": "socks5",
    "host": "",
    "port": 1080,
    "authentication": false,
    "credentialRef": "proxy-uuid"
  }
}
```

普通配置参与 SaveCurrent、LoadByName、SaveAs、默认配置和 `SaveForUnload` 的现有保存机制。凭据引用指向用户本机受保护的独立存储，用户名/密码不明文导出。复制配置可共享 credentialRef，删除引用时做引用计数；不能删除一个配置就破坏其他配置凭据。

利用 Windows DPAPI CurrentUser 保护凭据，复用已有 accounts/CredentialStore 的思路，但抽取中立 helper 或建立 proxy 自己的封装，不让 core proxy 依赖 minecraft/accounts 命名空间。密文不能保证在另一台电脑或另一个 Windows 用户下解密，导入后显示“请重新填写认证”，失败关闭。

密码编辑临时缓冲不写日志，取消/保存/禁用/卸载时尽量清理；不能声称 std::string/ImGui 所有副本均可绝对擦除。

Proxy 不占用 Scaffold/KillAura 的转头、攻击或库存事务；不通过模块发包事件发送代理协议，不进入 AntiKB、Backtrack、AutoBlock 的队列。所有这些逻辑仍作用于 Minecraft 的原版 Netty 连接。

## 12. 文件结构与接口

```text
Client/src/proxy/
  ProxyConfig.h/.cpp       草稿校验、不可变快照、revision
  ProxyService.h/.cpp      active tunnels、测试任务、状态发布、停止
  ProxyTypes.h            endpoint、error、phase、cancel
  ProxyCredentials.h/.cpp DPAPI 与 credential reference
  SocketTransport.h/.cpp  DNS、非阻塞socket、deadline和取消
  HttpConnect.h/.cpp      增量HTTP CONNECT状态机
  Socks5.h/.cpp           增量SOCKS5状态机
  TcpRelay.h/.cpp         loopback、身份验证、双向泵及半关闭
  ProxyConfigCodec.h/.cpp 配置序列化
  CMakeLists.txt

Client/src/contracts/ProxyService.h
Client/src/ui/clickgui/proxy/ProxyPage.h/.cpp

Client/src/minecraft/proxy/
  MinecraftProxyAdapter.h/.cpp
  JoinAttempt.h/.cpp
  ProxyHooks.h/.cpp
  ProxyEdits.h/.cpp
  mapping/Legacy.h
  mapping/Forge1201.h
  mapping/Lunar12111.h
  mapping/EaseCation.h
  mapping/Minecraft263.h
```

核心服务建议接口：`Apply(config, credentials)`、`Snapshot()`、`Test(target)`、`CancelTest()`、`PrepareTunnel(target, configSnapshot, cancel)`、`BeginShutdown()`、`JoinStoppedWorkers()`。

UI 只依赖 contracts，不能自行获取 Minecraft/JNIEnv。Minecraft adapter 负责 JoinAttempt、版本能力和原版取消/错误反馈。核心 service 不调用 Minecraft 类型。

## 13. 生命周期与卸载

开启：只激活配置，不立即连接代理。新加入：快照化配置并创建 tunnel。游戏断开：下游关闭触发 relay drain/close。关闭开关：下一次直连；当前连接不突然断开。

DEL 熔断：

1. SaveForUnload 保存当前/默认配置。
2. 停止接纳新加入与测试，取消准备中 attempt。
3. 在允许的游戏线程请求代理游戏 Connection 断开，避免原版无限等待。
4. 关闭所有 relay/socket，唤醒 native worker，等待退出并释放 JNI 引用。
5. 确认没有执行中的 proxy hook/worker，再恢复钩子与卸载 DLL。

如果 worker 或回调无法按期限排空，沿用现有安全卸载策略：保留 DLL 但停止服务，报告具体失败；不能带着执行中的原生 relay 把 DLL unmap。熔断后不承诺保持代理游戏连接，因为其 TCP 会话无法移交到直连而保持服务器状态。

## 14. 映射验证与测试计划

### 14.1 实施前映射审计

- 每个目标 JAR 检查连接工厂描述符及 Bootstrap.connect 调用次数，核对 join scope、取消、失败和异常出口。
- Lunar 运行时 Mixin 可能移动调用点：检查 bake 与运行时 dump；不能沿用随机 Mixin 后缀。
- 编写原生 classfile 改写测试，核对 branch、exception table、StackMapTable；不生成 Java 测试类。
- 26.3 网络连接和 SDL 生命周期独立审计，完成前标记 Proxy capability 不可用。
- 支持能力由 profile 发布；代理启用但未支持的版本必须阻止加入并显示原因，而不是静默直连。

### 14.2 纯原生协议测试

HTTP：分段状态行/头部、200/其他2xx、407/403/5xx、1xx、错误状态行、头部超限、IPv6、认证注入拒绝、成功头后的残余字节。

SOCKS5：00/02/FF、错误版本、认证失败、UTF8字节上限、IPv4/IPv6/domain、分段响应、每种REP、未知ATYP、截断数据、错误RSV。

Relay：随机分段与partial send、双向并发、超过缓冲上限的反压、FIN排空、RST、取消、陌生本地PID拒绝、监听超时、多attempt互不串流、worker退出与socket泄漏。

用 C++ 本地假代理/echo server 提供确定性成功与失败，无需真人账号、外部服务器或修改全局网络。

### 14.3 游戏集成验收

- 所有支持版本分别测试：直连基线、HTTP、SOCKS5、有/无认证、断开重连、取消、错误代理、代理中断。
- 本地自建 Minecraft 服务器记录到达源和逻辑握手主机，确认没有把localhost作为虚拟主机。
- 代理端抓包确认游戏只连接代理；用假的认证失败代理确认不出现目标直连。
- 验证 SRV 非25565、域名、IPv6、单人LocalChannel和现代 Transfer。
- 验证加密登录、压缩开启/变化、较大区块流、长时间keepalive，无丢包或乱序。
- 验证 AntiKB/AutoBlock 等原有包队列只处理 Minecraft 包，不接触代理认证帧。
- 实测本地relay相比直连的吞吐、CPU、内存和延迟；不以 Test Connection 耗时替代游戏RTT。
- 测试 ClickGUI/F11、切配置、输入密码、DPAPI不可解密、熔断和再次注入。

通过协议测试与构建只能证明实现可编译和本地逻辑正确；版本支持完成必须有该版本实际加入记录。

## 15. 分阶段实施与完成标准

1. **版本钩子验证**：优先 Lunar 1.8.9、1.21.11，确认 transport redirect、JoinAttempt范围和取消；保留其他版本明确 capability。
2. **原生协议与relay**：HTTP/SOCKS5、取消、缓冲、半关闭、loopback身份检查、fake proxy集成测试。
3. **UI和配置**：Proxy大分类、现有风格控件、凭据保护、Apply/Test、SaveForUnload。
4. **Minecraft接入**：保留原版Bootstrap初始化器，代理准备完成后重定向transport，验证登录host及失败关闭。
5. **扩展版本**：1.7、Forge/EaseCation、26.3分别审计并验收，不以名字相似直接宣布支持。
6. **后续可选**：Remote DNS、服务器列表ping代理、HTTPS代理；每项有独立协议与范围定义。

首版完成标准：Proxy页面可手动配置两种协议；认证和测试可用；加入流量经代理；失败不直连；取消响应正常；加密/压缩/重连正常；配置与熔断处理正确；已支持版本有实际验证。其他扩展不作为首版的隐式承诺。

## 16. 对初步口头方案的修正

先前建议的“在Netty中加代理握手处理器”和“SOCKS5目标域名直接交给代理”是初步方向。Recaf核对后确认原版提前解析IP、登录立即跟随连接，且项目不能新增Java handler，故详细方案改为原生中继；首版保留本地DNS/SRV，并将Remote DNS列为独立后续适配。文档没有把这些尚未实现的能力描述成当前功能。

## 17. 首版实际实现与使用

入口为 Panel ClickGUI 左侧 Proxy 大分类。填写 HTTP (CONNECT) / SOCKS5、Host、Port，可选 Authentication、Username、Password，然后 Apply。开启 Enabled 并 Apply 后，下次点击加入服务器使用本次配置快照；关闭 Enabled 并 Apply 恢复后续直连。现有连接保持到原版断开或熔断。

Test Connection 使用页面填写的 Target Host/Port，与正式加入相同的原生协议和socket实现。测试成功只表示代理隧道能建立，不表示账号能够登录该Minecraft服务器；认证失败和目标拒绝会显示明确错误。

已接入 Lunar 1.7.10、1.8.9、1.21.11、Forge 1.20.1、EaseCation 1.21.8 基础网络ABI及Minecraft 26.3。六种实际JAR进行本次连接钩子的离线改写检查；EaseCation运行时Mixin和所有版本真实加入仍需验证。CS2尚未接入，此时页面禁止启用，加载启用配置会明确报告该版本不可用并关闭代理，不能宣称代理正在生效。

实际文件分为 `src/proxy/ProxyService.*`（配置、协议、native relay与测试任务）、`src/minecraft/proxy/ProxyHooks.*` / `ProxyEdits.cpp`（本次加入作用域、取消标记与Bootstrap transport重定向）、`src/ui/clickgui/proxy/ProxyPage.*`（共享UI）。为保持首版紧凑，协议和relay尚未拆成独立编译单元。

凭据首版保存为当前配置 `proxy.credentials` 的DPAPI加密十六进制blob，而非单独的credentialRef存储。仍不保存明文用户名/密码，不依赖账号模块，并且沿用Config.Save/Load和SaveForUnload。跨Windows用户/设备不可解密时保留代理启用请求并使连接失败，要求重新填写认证。与第11节的分离引用方案相比，此实现复制配置即可复制密文，但不会带来跨设备可解密能力。

原版连接线程先等待native代理握手完成，再调用原版Bootstrap连接本次loopback端口。取消通过原版ConnectScreen的volatile取消字段每50毫秒检测，无需修改取消按钮；这个等待不运行在render或Netty event loop。代理协议握手不进Minecraft事件队列，原版TCP管线、目标握手host和加密/压缩保持原有路径。

测试入口：`Client/test/proxy/run.ps1`，包括真实原生HTTP/SOCKS5假代理、可选认证、IPv4/IPv6/域名、分段回复、成功头后残余字节、1MiB双向relay、半关闭、拒绝、取消、DPAPI往返；真实JNI适配器的scope/disabled/status-ping区分、loopback重定向、失败不调用原版直连、异常引用释放；六版本JAR精确Bootstrap调用点及run入口/返回改写。页面预览：`NovaUiPreview.exe --proxy-smoke`，生成`proxy-page.bmp`。上述测试和Release构建均通过；构建产物仍需用户重新注入并用实际代理验证游戏加入。

熔断会停止本次中继，因而代理游戏连接会断开。不能在DLL卸载后保留依赖该DLL的relay，也不会在此时自动直连。

### 17.1 Lunar 1.8.9 规则合并修复

首次运行时报“selected client event rules did not all apply”：日志显示NetworkManager连接工厂的Proxy规则重复执行。账号适配器原先从AllEdits读取已扩展规则，又设置为custom table，最终Proxy再次扩展；第一次已替换Bootstrap.connect，第二次没有原调用可匹配。

账号组合改为读取BaseEdits；ExtendEdits检测完整Proxy规则集合并保持幂等，不重复追加也不清空调用方借用的底层存储。新增重复账号组合测试，并使用本次失败的 `nova-hook-in` 运行时class验证。检查仍严格要求全部规则应用，没有放宽注入完成判定。失败日志明确保留惰性DLL，因此用户需重启游戏后再验证修复版注入。

第二次在新进程主菜单注入时，日志为 expected=22、retransformed=21、rewriteFailed=0：连接线程内部类尚未加载。安装前预加载原先只对modern profile执行，现改为所有profile的最终必需规则表都经游戏ClassLoader.loadClass预加载，再开始retransform。新增主菜单lazy-target测试，验证缺失类仍然失败、异常清理及JNI引用释放；严格的22类完成判定没有削弱。

### 17.2 本机加速器 Fake-IP 兼容

实测本机DNS将mc.loyisa.cn解析为198.18.1.6及2001:2::105。对127.0.0.1:1080请求域名返回SOCKS5 REP=0，请求198.18.1.6返回REP=4。加入时捕获旧版连接线程中的原始主机名；遇到198.18.0.0/15或2001:2前缀虚拟地址时，向代理发送域名，其他实际IP保留现有行为。现代版无旧版线程String字段时尝试InetAddress保留的hostname；无法取得域名则失败关闭。SOCKS5失败现在显示具体REP和含义。此兼容避免向代理请求无法路由的Fake-IP，不等于删除原版DNS/SRV查询或宣称完全远程DNS。
