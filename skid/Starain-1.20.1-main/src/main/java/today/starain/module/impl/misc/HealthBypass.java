package today.starain.module.impl.misc;

import net.minecraft.client.Minecraft;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.network.protocol.game.ClientboundSetScorePacket;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.PacketEvent;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

public class HealthBypass extends ClientModule {
    public static final Map<String, AtomicInteger> HEALTHS = new HashMap<>();

    public HealthBypass() {
        setName("HealthBypass");
        setCategory(Category.MISC);
    }

    @EventTarget
    public void onPacket(PacketEvent event) {
        if (event.getPacket() instanceof ClientboundSetScorePacket packet
                && Minecraft.getInstance().level != null
                && ("belowHealth".equals(packet.getObjectiveName()) || "health".equals(packet.getObjectiveName()))
                && !packet.getOwner().equals(Minecraft.getInstance().player.getGameProfile().getName())) {
            if (!HEALTHS.containsKey(packet.getOwner())) {
                AtomicInteger atomic = new AtomicInteger();
                HEALTHS.put(packet.getOwner(), atomic);
            }

            HEALTHS.get(packet.getOwner()).set(packet.getScore());
        }
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {
        for (AbstractClientPlayer player : Minecraft.getInstance().level.players()) {
            if (player != Minecraft.getInstance().player && HEALTHS.containsKey(player.getName().getString())) {
                player.setHealth((float) Math.max(0.01, HEALTHS.get(player.getName().getString()).get()));
            }
        }
    }
}
