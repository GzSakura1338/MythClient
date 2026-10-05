package dev.jiuxian.common.packet.impl.s2c;

import dev.jiuxian.common.packet.Packet;
import lombok.Getter;

@Getter
public class SystemMsgS2CPacket extends Packet {
    private final String message;

    public SystemMsgS2CPacket(String message) {
        super("SystemMsgS2CPacket");
        this.message = message;
    }
}
