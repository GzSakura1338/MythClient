package dev.jiuxian.server.socket;

import dev.jiuxian.common.packet.impl.c2s.ChatC2SPacket;
import dev.jiuxian.common.packet.impl.c2s.LoginC2SPacket;
import dev.jiuxian.common.packet.impl.c2s.RegisterC2SPacket;
import dev.jiuxian.common.packet.impl.c2s.UpdateIgnC2SPacket;
import dev.jiuxian.common.packet.impl.s2c.ChatS2CPacket;
import dev.jiuxian.common.packet.impl.s2c.SyncUsersS2CPacket;
import dev.jiuxian.common.packet.impl.s2c.SystemMsgS2CPacket;
import dev.jiuxian.common.packet.impl.s2c.UserInfoS2CPacket;
import dev.jiuxian.server.entity.Key;
import dev.jiuxian.server.entity.User;
import dev.jiuxian.common.packet.Packet;
import dev.jiuxian.server.database.DatabaseManager;
import dev.jiuxian.server.manager.SessionManager;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;
import io.netty.channel.group.ChannelGroup;
import io.netty.channel.group.DefaultChannelGroup;
import io.netty.util.concurrent.GlobalEventExecutor;
import lombok.extern.slf4j.Slf4j;

@Slf4j
public class ServerHandler extends SimpleChannelInboundHandler<Packet> {
    public static final ChannelGroup channels = new DefaultChannelGroup(GlobalEventExecutor.INSTANCE);
    private boolean authenticated = false;

    @Override
    protected void channelRead0(ChannelHandlerContext ctx, Packet msg) throws Exception {
        if (!authenticated) {
            if (msg instanceof LoginC2SPacket login) {
                String username = login.getUsername();
                String password = login.getPassword();
                String qq = login.getQq();
                String hwid = login.getHwid();

                User user = DatabaseManager.getUserDao().queryBuilder()
                        .where()
                        .eq("username", username)
                        .queryForFirst();

                if (user == null) {
                    ctx.channel().writeAndFlush(new UserInfoS2CPacket("用户名或密码错误", false, 0));
                    return;
                }

                if (!user.getPassword().equals(password)) {
                    ctx.channel().writeAndFlush(new UserInfoS2CPacket("用户名或密码错误", false, 0));
                    return;
                }

                if (!user.getHwid().equals(hwid)) {
                    ctx.channel().writeAndFlush(new UserInfoS2CPacket("设备码不匹配", false, 0));
                    return;
                }

                if (user.isBanned()) {
                    ctx.channel().writeAndFlush(new UserInfoS2CPacket("用户已被封禁", true, user.getBanExpiryDate()));
                    return;
                }

                user.setQq(qq);
                DatabaseManager.getUserDao().update(user);
                SessionManager.addSession(username, hwid, qq, ctx.channel());
                channels.add(ctx.channel());

                ctx.channel().writeAndFlush(new UserInfoS2CPacket(user.getExpiryDate(), SessionManager.getToken(ctx.channel())));
                channels.writeAndFlush(new SystemMsgS2CPacket("§b" + username + "§f上线了"));
                authenticated = true;
                return;
            }
            if (msg instanceof RegisterC2SPacket register) {
                String username = register.getUsername();
                String password = register.getPassword();
                String cardKey = register.getCardKey();
                String qq = register.getQq();
                String hwid = register.getHwid();

                Key key = DatabaseManager.getKeyDao().queryBuilder()
                        .where()
                        .eq("content", cardKey)
                        .queryForFirst();
                if (key == null) {
                    ctx.channel().writeAndFlush(new UserInfoS2CPacket("卡密无效", false, 0));
                    return;
                }

                User user = DatabaseManager.getUserDao().queryBuilder()
                        .where()
                        .eq("username", username)
                        .queryForFirst();

                if (user == null) {
                    user = new User();
                    user.setUsername(username);
                    user.setPassword(password);
                    user.setHwid(hwid);
                    user.setQq(qq);
                    user.setExpiryDate(System.currentTimeMillis() + key.getDuration());
                    DatabaseManager.getUserDao().create(user);
                } else {

                    if (!user.getPassword().equals(password)) {
                        ctx.channel().writeAndFlush(new UserInfoS2CPacket("用户名冲突或密码错误", false, 0));
                        return;
                    }

                    if (!user.getHwid().equals(hwid)) {
                        ctx.channel().writeAndFlush(new UserInfoS2CPacket("设备码不匹配", false, 0));
                        return;
                    }

                    if (user.isBanned()) {
                        ctx.channel().writeAndFlush(new UserInfoS2CPacket("用户已被封禁", true, user.getBanExpiryDate()));
                        return;
                    }
                    user.setQq(qq);
                    user.setExpiryDate(Math.max(System.currentTimeMillis() + key.getDuration(), user.getExpiryDate() + key.getDuration()));
                    DatabaseManager.getUserDao().update(user);
                }
                key.setUsed(true);
                key.setUsedBy(user);
                DatabaseManager.getKeyDao().update(key);

                SessionManager.addSession(username, hwid, qq, ctx.channel());
                channels.add(ctx.channel());

                ctx.channel().writeAndFlush(new UserInfoS2CPacket(user.getExpiryDate(), SessionManager.getToken(ctx.channel())));
                channels.writeAndFlush(new SystemMsgS2CPacket("§b" + username + "§f上线了"));
                authenticated = true;
                return;
            }
            return;
        }

        if (msg instanceof UpdateIgnC2SPacket update) {
            SessionManager.setIgn(update.getIgn(), ctx.channel());
            channels.writeAndFlush(new SyncUsersS2CPacket(SessionManager.getUser2IgnMap()));
            return;
        }

        if (msg instanceof ChatC2SPacket chat) {
            String username = SessionManager.getUsername(ctx.channel());
            channels.writeAndFlush(new ChatS2CPacket(chat.getMessage(), username));
            return;
        }


    }

    @Override
    public void channelInactive(ChannelHandlerContext ctx) throws Exception {
        SessionManager.removeSession(ctx.channel());
        super.channelInactive(ctx);
        String username = SessionManager.getUsername(ctx.channel());
        if (username != null) {
            channels.writeAndFlush(new SystemMsgS2CPacket("§b" + username + "§f跑路了"));
        }
    }
    
    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) {
        // cause.printStackTrace(); // Reduce log noise
        ctx.close();
    }
}
