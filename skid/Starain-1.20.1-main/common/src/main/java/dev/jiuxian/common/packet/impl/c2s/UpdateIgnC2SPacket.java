package dev.jiuxian.common.packet.impl.c2s;

import dev.jiuxian.common.packet.Packet;
import lombok.Getter;

@Getter
public class UpdateIgnC2SPacket extends Packet {
    private final String ign;
    public UpdateIgnC2SPacket(String ign) {
        super("UpdateIgnC2SPacket");
        this.ign = ign;
    }
}
