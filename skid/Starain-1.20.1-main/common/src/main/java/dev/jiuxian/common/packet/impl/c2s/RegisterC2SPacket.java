package dev.jiuxian.common.packet.impl.c2s;

import dev.jiuxian.common.packet.Packet;
import lombok.Getter;

@Getter
public class RegisterC2SPacket extends Packet {

    private final String username;
    private final String password;
    private final String cardKey;
    private final String hwid;
    private final String qq;

    public RegisterC2SPacket(String username, String password, String cardKey, String hwid, String qq) {
        super("RegisterC2SPacket");
        this.username = username;
        this.password = password;
        this.cardKey = cardKey;
        this.hwid = hwid;
        this.qq = qq;
    }
}
