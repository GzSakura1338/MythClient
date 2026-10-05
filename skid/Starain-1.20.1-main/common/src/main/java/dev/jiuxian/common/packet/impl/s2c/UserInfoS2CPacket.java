package dev.jiuxian.common.packet.impl.s2c;

import dev.jiuxian.common.packet.Packet;
import lombok.Getter;

@Getter
public class UserInfoS2CPacket extends Packet {
    private final boolean success;
    private final String message;
    private final boolean banned;
    private final long expiryDate;
    private final long banExpiryDate;
    private final String token;

    public UserInfoS2CPacket(long expiryDate, String token) {
        super("UserInfoS2CPacket");
        success = true;
        message = "";
        this.banned = false;
        this.expiryDate = expiryDate;
        this.banExpiryDate = 0;
        this.token = token;
    }

    public UserInfoS2CPacket(String message, boolean banned, long banExpiryDate) {
        super("UserInfoS2CPacket");
        success = false;
        this.message = message;
        this.banned = banned;
        this.expiryDate = 0;
        this.banExpiryDate = banExpiryDate;
        this.token = "";
    }
}
