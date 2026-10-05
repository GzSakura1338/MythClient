package today.starain.utils.client;

import net.minecraft.network.protocol.Packet;
import today.starain.utils.wrapper.Wrapper;

import java.util.HashSet;
import java.util.Set;

public class NetworkUtils implements Wrapper {
    public static Set<Packet<?>> passthroughsPackets = new HashSet<>();


    public static void sendPacketNoEvent(Packet<?> packet) {
        passthroughsPackets.add(packet);
        mc.getConnection().send(packet);
    }
}
