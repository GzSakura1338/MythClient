package dev.jiuxian.server.socket;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import dev.jiuxian.common.packet.Packet;
import dev.jiuxian.common.packet.impl.c2s.ChatC2SPacket;
import dev.jiuxian.common.packet.impl.c2s.LoginC2SPacket;
import dev.jiuxian.common.packet.impl.c2s.RegisterC2SPacket;
import dev.jiuxian.common.packet.impl.c2s.UpdateIgnC2SPacket;
import dev.jiuxian.common.packet.impl.s2c.ChatS2CPacket;
import dev.jiuxian.common.packet.impl.s2c.SyncUsersS2CPacket;
import dev.jiuxian.common.packet.impl.s2c.SystemMsgS2CPacket;
import dev.jiuxian.common.packet.impl.s2c.UserInfoS2CPacket;
import dev.jiuxian.common.utils.EncryptionUtil;
import dev.jiuxian.server.config.Config;
import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.ByteToMessageCodec;
import lombok.extern.slf4j.Slf4j;

import java.nio.charset.StandardCharsets;
import java.util.List;

import static dev.jiuxian.common.utils.EncryptionUtil.getOnlineTime;

@Slf4j
public class PacketCodec extends ByteToMessageCodec<Packet> {
    private static final Gson gson = new Gson();

    @Override
    protected void encode(ChannelHandlerContext ctx, Packet msg, ByteBuf out) throws Exception {
        msg.setTime(Long.parseLong(getOnlineTime("https://www.baidu.com")));
        String json = gson.toJson(msg);
        String encrypted = EncryptionUtil.encrypt(json, Config.get().getAesSecret());
        byte[] bytes = encrypted.getBytes(StandardCharsets.UTF_8);
        out.writeBytes(bytes);
    }

    @Override
    protected void decode(ChannelHandlerContext ctx, ByteBuf in, List<Object> out) throws Exception {
        byte[] bytes = new byte[in.readableBytes()];
        in.readBytes(bytes);
        String encrypted = new String(bytes, StandardCharsets.UTF_8);
        try {
            String json = EncryptionUtil.decrypt(encrypted, Config.get().getAesSecret());
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            if (System.currentTimeMillis() - obj.get("time").getAsLong() >= 30000) {
                log.error("Failed to decode packet", new Exception("timed out."));
                ctx.close();
            }
            String type = obj.get("type").getAsString();
            
            Packet packet = null;
            switch (type) {
                case "ChatC2SPacket": packet = gson.fromJson(json, ChatC2SPacket.class); break;
                case "LoginC2SPacket": packet = gson.fromJson(json, LoginC2SPacket.class); break;
                case "RegisterC2SPacket": packet = gson.fromJson(json, RegisterC2SPacket.class); break;
                case "UpdateIgnC2SPacket": packet = gson.fromJson(json, UpdateIgnC2SPacket.class); break;
                case "ChatS2CPacket": packet = gson.fromJson(json, ChatS2CPacket.class); break;
                case "SyncUsersS2CPacket": packet = gson.fromJson(json, SyncUsersS2CPacket.class); break;
                case "SystemMsgS2CPacket": packet = gson.fromJson(json, SystemMsgS2CPacket.class); break;
                case "UserInfoS2CPacket": packet = gson.fromJson(json, UserInfoS2CPacket.class); break;
            }
            
            if (packet != null) {
                out.add(packet);
            }
        } catch (Exception e) {
            log.error("Failed to decode packet", e);
            ctx.close();
        }
    }
}
