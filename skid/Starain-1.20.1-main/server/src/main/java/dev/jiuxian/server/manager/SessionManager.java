package dev.jiuxian.server.manager;

import io.netty.channel.Channel;
import io.netty.util.AttributeKey;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import dev.jiuxian.common.utils.EncryptionUtil;
import dev.jiuxian.server.config.Config;

public class SessionManager {
    public static final Map<String, Channel> activeSessions = new ConcurrentHashMap<>();
    private static final AttributeKey<String> USERNAME_KEY = AttributeKey.valueOf("username");
    private static final AttributeKey<String> IGN_KEY = AttributeKey.valueOf("ign");
    private static final AttributeKey<String> TOKEN_KEY = AttributeKey.valueOf("token");

    public static void addSession(String username, String hwid, String qq, Channel channel) {
        if (activeSessions.containsKey(username)) {
            Channel old = activeSessions.get(username);
            if (old.isActive()) old.close();
        }
        activeSessions.put(username, channel);
        channel.attr(USERNAME_KEY).set(username);

        String maxQQ = "0";
        if (qq != null && !qq.isEmpty()) {
            String[] qqs = qq.split(",");
            long maxVal = -1;
            for (String q : qqs) {
                try {
                    long val = Long.parseLong(q.trim());
                    if (val > maxVal) {
                        maxVal = val;
                        maxQQ = q.trim();
                    }
                } catch (Exception ignored) {
                }
            }
        }

        String aesSecret = Config.get().getAesSecret();
        String token = generateToken(hwid, maxQQ, aesSecret);
        channel.attr(TOKEN_KEY).set(token);
    }

    private static String generateToken(String hwid, String maxQQ, String aesSecret) {
        byte[] hwidBytes = (hwid == null ? "" : hwid).getBytes(StandardCharsets.UTF_8);
        byte[] qqBytes = (maxQQ == null ? "0" : maxQQ).getBytes(StandardCharsets.UTF_8);
        byte[] result = new byte[hwidBytes.length];
        for (int i = 0; i < hwidBytes.length; i++) {
            result[i] = (byte) (hwidBytes[i] ^ qqBytes[i % qqBytes.length]);
        }
        String xorString = Base64.getEncoder().encodeToString(result);
        try {
            return EncryptionUtil.encrypt(xorString, aesSecret);
        } catch (Exception e) {
            return "";
        }
    }

    public static void setIgn(String ign, Channel channel) {
        channel.attr(IGN_KEY).set(ign);
    }

    public static String getIgn(Channel channel) {
        if (!channel.hasAttr(IGN_KEY)) return null;
        return channel.attr(IGN_KEY).get();
    }

    public static String getUsername(Channel channel) {
        if (!channel.hasAttr(USERNAME_KEY)) return null;
        return channel.attr(USERNAME_KEY).get();
    }

    public static Map<String, String> getUser2IgnMap() {
        Map<String, String> user2Ign = new HashMap<>();
        activeSessions.forEach((username, channel) -> {
            user2Ign.put(username, channel.attr(IGN_KEY).get());
        });
        return user2Ign;
    }

    public static String getToken(Channel channel) {
        if (channel.hasAttr(TOKEN_KEY)) {
            return channel.attr(TOKEN_KEY).get();
        }
        return null;
    }

    public static void setToken(String token, Channel channel) {
        channel.attr(TOKEN_KEY).set(token);
    }

    public static void removeSession(Channel channel) {
        if (channel.hasAttr(USERNAME_KEY)) {
            String username = channel.attr(USERNAME_KEY).get();
            if (username != null) {
                activeSessions.remove(username);
            }
        }
    }

    public static void kickUser(String username) {
        if (activeSessions.containsKey(username)) {
            Channel channel = activeSessions.get(username);
            channel.close();
            activeSessions.remove(username);
        }
    }
    
    public static boolean isOnline(String username) {
        return activeSessions.containsKey(username);
    }
}
