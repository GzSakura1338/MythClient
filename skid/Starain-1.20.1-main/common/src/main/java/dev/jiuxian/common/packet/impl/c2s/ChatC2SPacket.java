package dev.jiuxian.common.packet.impl.c2s;

import dev.jiuxian.common.packet.Packet;
import lombok.Getter;

@Getter
public class ChatC2SPacket extends Packet {
    private final String message;

    public ChatC2SPacket(String message) {
        super("ChatC2SPacket");
        this.message = message;
    }
}
