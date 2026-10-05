# Microsoft 浏览器登录

Nova 的 Accounts → Add Microsoft，以及已适配的 Lunar 原生账号页“添加账号”，直接打开 Windows 默认浏览器。微软授权返回 Minecraft 进程，由 Nova 完成 Xbox Live、XSTS、Minecraft 登录和会话切换，不调用 Lunar Launcher 登录窗口。

当前接管范围是 **Lunar 1.21.11 的已核验 bake**。必须在游戏主菜单完成登录。新的正版账号保存在 Nova 的账号列表中，不写入启动器的账号文件。

## 默认免填写登录

主菜单点击 **Add Microsoft** 即打开系统默认浏览器，不再弹出 Client ID 设置。默认使用本机 IAS 9.0.8 中的公开应用 ID `54fd49e4-2103-4044-9603-2b028c814ec3`，没有应用密钥。页头 Microsoft / Restore 两个按钮已移除。

授权端点为 `https://login.live.com/oauth20_authorize.srf`，兑换端点为 `https://login.live.com/oauth20_token.srf`。默认请求 `XboxLive.signin XboxLive.offline_access`，采用 IAS 的完整注册回调路径和候选端口列表，从 59125 开始尝试。监听只绑定本机回环地址；实际绑定端口用于授权及兑换，回调校验随机 state、路径和长度。IAS 默认链路不额外附加其参考实现未使用的 PKCE 参数。

已保存的自定义 Client ID 仍生效，已有微软账号继续使用保存时的应用 ID 刷新。元数据 `clientId` 留空使用内置默认值；不必配置。微软网页的应用名称由应用注册决定，本地 Settings 客户端名称不会修改微软网页上的身份标识。

## 可选自定义应用（兼容已有配置）

1. 打开 [Microsoft Entra 管理中心](https://entra.microsoft.com/)，进入 **App registrations → New registration**。需要有创建应用注册权限；如果个人账号无法进入，需先获得可用租户和对应权限。
2. 创建你自己的应用，选择支持 **Personal Microsoft accounts** 的账号类型，不适用于仅组织账号的应用。应用注册名称可以设置为 NOVA；微软授权网页的名称取自这项注册，不会随本地 Settings 自动改变。
3. 在 **Authentication → Add a platform → Mobile and desktop applications** 配置公开客户端回调：`http://localhost:8247`。使用 HTTP localhost 回环地址；不要添加 `/callback`，不要配置成 Web confidential client。旧版 Nova 使用 39185，升级后需要修改注册回调。
4. 在应用的认证设置中启用适用的公开客户端能力（Allow public client flows）。不需要创建或填写 Client Secret；客户端内不能保管应用密钥。
5. 按 Minecraft 当前第三方应用接入规则，确认该应用具有 Xbox/Minecraft 服务访问资格。**注册出 Client ID 不代表已获 Minecraft API 许可**；如需审批，须完成审批后使用。以 [Minecraft 官方帮助中心](https://help.minecraft.net/) 当前提供的 Java Edition third-party application / authentication 指引为准。
6. 可在 Nova 未运行时将公开 Application (client) ID 配入账号元数据的 `clientId` 字段。这里只填写形如 `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx` 的公开 ID，不填写密码或密钥；界面不再提供设置按钮。
7. 点击 **Add Microsoft**，在浏览器选择账号并授权，然后回到游戏等待切换完成。

自定义应用使用 Authorization Code + PKCE S256，请求 `XboxLive.signin offline_access`。这些 scope 在授权请求中发送，不是 Microsoft Graph 的 User.Read 权限。

自定义应用回调仍为 `http://localhost:8247`，随后经过 Xbox Live → XSTS → Minecraft profile。保留超时、取消和 DPAPI；没有复制 Rise 的嵌入式应用密钥。

官方参考：[应用注册](https://learn.microsoft.com/en-us/entra/identity-platform/quickstart-register-app)、[回调地址规则](https://learn.microsoft.com/en-us/entra/identity-platform/reply-url)、[授权码流程和 PKCE](https://learn.microsoft.com/en-us/entra/identity-platform/v2-oauth2-auth-code-flow)。

## 登录和保存

- 已保存的 Nova 微软账号优先使用自己的 refresh token；刷新凭据过期或被撤销时重新打开浏览器。
- Lunar 已有有效账号可以先使用其当前凭据切换；需要重新授权的条目转入浏览器流程。没有自定义 Client ID 时使用内置应用，不回退到启动器。
- 可以点击 **Cancel sign-in** 取消。浏览器回调最多等待 5 分钟；取消、网络失败或授权失败不会提交新身份。
- 浏览器出现“Sign-in received”仅表示 Nova 收到授权码，最终结果以游戏账号页为准。
- Client ID 是公开配置。refresh token 使用 Windows 当前用户的 DPAPI 加密后保存在 `%LOCALAPPDATA%/Nova/Accounts/metadata.json`，access token 不写入该文件；普通模块配置不包含这些凭据。换电脑需重新登录。
- 移除 Nova 账号不注销当前游戏会话，也不删除 Launcher 中的账号。熔断会停止当前授权任务；已完成切换的身份保持不变。

## 常见问题

| 提示或现象 | 处理 |
| --- | --- |
| 点击仍要求填写 Client ID | 确认已卸载旧实例并注入本次 DLL |
| redirect URI mismatch / AADSTS50011 | 核对公开客户端平台和 `http://localhost:8247` |
| Browser sign-in ports are busy | 关闭其他本地授权任务后重试；默认会尝试多个候选端口 |
| 浏览器成功但 Minecraft sign-in failed | 检查应用的 Minecraft API 资格、账号 Xbox 档案和 Java 版访问资格 |
| 选择已有账号后提示账号不匹配 | 浏览器中选择该条目对应的微软账号；要换另一账号请用 Add Microsoft |
| Minecraft 中仍打开 Launcher 登录 | 确认已熔断旧 DLL、注入新构建，且当前运行的是已适配的 Lunar 1.21.11 |

验证范围包含编译、原始 Lunar 类重定向、授权参数、回调解析、DPAPI 和空 Client ID 的界面预览；没有替用户执行真实微软授权。内置应用当前服务端可用性与完整账号切换仍需实际登录确认。

## 离线账号进入游戏

Lunar 的 setScreen 会把启动器无选中账号视为未登录，原本导致 Nova 离线身份点击单人/多人时再次打开账号页。此外，多人界面打开事件还会独立检查一次登录状态并取消界面切换；只修 setScreen 无法解决这第二层检查。现在两处均识别已提交的 Nova 会话，核对当前 Minecraft User 的 UUID、名字和账号状态；事件修复仅作用于原始代码中已通过多人界面类型判断的调用点。Realms、Lunar 服务及服务器认证逻辑保持原规则。离线身份用于单人和允许离线身份的服务器，不提供正版服务器登录凭据。
