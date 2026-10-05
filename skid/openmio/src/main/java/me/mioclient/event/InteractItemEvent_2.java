package me.mioclient.event;

import net.minecraft.network.packet.c2s.play.PlayerInteractItemC2SPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/InteractItemEvent_2.class */
public class InteractItemEvent_2 extends Event {
    public PlayerInteractItemC2SPacket playerInteractItemC2SPacket;

    public InteractItemEvent_2(PlayerInteractItemC2SPacket playerInteractItemC2SPacket) {
        this.playerInteractItemC2SPacket = playerInteractItemC2SPacket;
    }

    public PlayerInteractItemC2SPacket getPlayerInteractItemC2SPacket1816() {
        return this.playerInteractItemC2SPacket;
    }

    public void do1817(PlayerInteractItemC2SPacket playerInteractItemC2SPacket) {
        this.playerInteractItemC2SPacket = playerInteractItemC2SPacket;
    }

    public void do1818(float[] fArr) {
        do1817(new PlayerInteractItemC2SPacket(this.playerInteractItemC2SPacket.getHand(), this.playerInteractItemC2SPacket.getSequence(), fArr[0], fArr[1]));
        do1162();
    }
}
