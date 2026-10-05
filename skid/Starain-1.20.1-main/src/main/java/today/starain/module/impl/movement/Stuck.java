package today.starain.module.impl.movement;

import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.game.ServerboundMovePlayerPacket;
import net.minecraft.network.protocol.game.ServerboundUseItemOnPacket;
import net.minecraft.network.protocol.game.ServerboundUseItemPacket;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.PacketEvent;
import today.starain.event.impl.PlayerTickEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.client.NetworkUtils;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class Stuck extends ClientModule {

    private int ticks;
    private int stage;
    private final Queue<Packet<?>> packets = new ConcurrentLinkedQueue<>();

    public Stuck() {
        setName("Stuck");
        setCategory(Category.MOVEMENT);
    }

    @Override
    protected void onEnable() {
        ticks = 20;
        stage = 0;
        packets.clear();
    }

    @Override
    protected void onDisable() {
        packets.clear();
    }

    @EventTarget
    public void onPacket(PacketEvent event) {
        if (event.getType() != EventType.OUTGOING) return;
        Packet<?> packet = event.getPacket();
        if (stage == 0) {
            if (packet instanceof ServerboundUseItemPacket || packet instanceof ServerboundUseItemOnPacket) {
                packets.add(packet);
                event.setCancelled(true);
                stage = 1;
            }
        }
    }

    @EventTarget
    public void onPlayerTick(PlayerTickEvent event) {
        if (mc.player == null) return;
        if (mc.player.onGround()) {
            setEnabled(false);
            return;
        }
        switch (stage) {
            case 0 -> {
                if (ticks > 0) {
                    event.setCancelled(true);
                    if (ticks == 10) {
                        NetworkUtils.sendPacketNoEvent(new ServerboundMovePlayerPacket.StatusOnly(mc.player.onGround()));
                    }
                    ticks--;
                } else {
                    ticks = 20;
                }
            }
            case 1 -> stage = 2;
            case 2 -> {
                while (!packets.isEmpty()) {
                    Packet<?> poll = packets.poll();
                    NetworkUtils.sendPacketNoEvent(poll);
                }
                stage = 0;
            }
        }
    }
}
