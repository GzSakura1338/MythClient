package me.mioclient;

import me.mioclient.mixin.ducks.DuckPlayerInteractEntityC2SPacket;
import net.minecraft.entity.Entity;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.util.Hand;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/LegacyCrystalSearchHelper4.class */
public class LegacyCrystalSearchHelper4 implements SearchHelper_4 {

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/LegacyCrystalSearchHelper4$LegacyCrystalMode.class */
    public enum LegacyCrystalMode {
        INTERACT,
        ATTACK,
        INTERACT_AT
    }

    public static Entity getEntity2610(PlayerInteractEntityC2SPacket playerInteractEntityC2SPacket) {
        return minecraftClient.world.getEntityById(((DuckPlayerInteractEntityC2SPacket) playerInteractEntityC2SPacket).getEntityId());
    }

    public static LegacyCrystalMode getLegacyCrystalMode2611(PlayerInteractEntityC2SPacket playerInteractEntityC2SPacket) {
        final LegacyCrystalMode[] mode = new LegacyCrystalMode[1];
        playerInteractEntityC2SPacket.handle(new PlayerInteractEntityC2SPacket.Handler() {
            @Override
            public void interact(Hand hand) {
                mode[0] = LegacyCrystalMode.INTERACT;
            }

            @Override
            public void interactAt(Hand hand, Vec3d vec3d) {
                mode[0] = LegacyCrystalMode.INTERACT_AT;
            }

            @Override
            public void attack() {
                mode[0] = LegacyCrystalMode.ATTACK;
            }
        });
        return mode[0];
    }
}
