package dev.jiuxian.common.packet.impl.s2c;

import dev.jiuxian.common.packet.Packet;
import lombok.Getter;

@Getter
public class ChatS2CPacket extends Packet {
    private final String message;
    private final String sender;

    public ChatS2CPacket(String message, String sender) {
        super("ChatS2CPacket");
        this.message = message;
        this.sender = sender;
    }
}
