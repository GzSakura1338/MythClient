package dev.jiuxian.common.packet.impl.s2c;

import dev.jiuxian.common.packet.Packet;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.Getter;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

@Getter
public class SyncUsersS2CPacket extends Packet {
    private final Map<String, String> users;

    public SyncUsersS2CPacket(Map<String, String> users) {
        super("SyncUsersS2CPacket");
        this.users = users;
    }
}
