package dev.jiuxian.common.packet.impl.c2s;

import dev.jiuxian.common.packet.Packet;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.Getter;

@Getter
public class LoginC2SPacket extends Packet {

    private final String username;
    private final String password;
    private final String hwid;
    private final String qq;

    public LoginC2SPacket(String username, String password, String hwid, String qq) {
        super("LoginC2SPacket");
        this.username = username;
        this.password = password;
        this.hwid = hwid;
        this.qq = qq;
    }
}
