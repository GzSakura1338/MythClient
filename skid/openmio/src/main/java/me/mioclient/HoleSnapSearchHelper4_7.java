package me.mioclient;

import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.module.movement.HoleSnap;
import me.mioclient.module.movement.Warp;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HoleSnapSearchHelper4_7.class */
public final class HoleSnapSearchHelper4_7 implements SearchHelper_4 {
    public static boolean flag;
    public static Warp warp = (Warp) BaritoneHelper_3.baritoneHelper_4.getModule117(Warp.class);
    public final HoleSnap holeSnap;
    public int num;

    public HoleSnapSearchHelper4_7(HoleSnap holeSnap) {
        this.holeSnap = holeSnap;
        baritoneHelper.do1796(this);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (is2181()) {
            return;
        }
        int intValue = warp.charge.getValue().intValue();
        int i = this.num + 1;
        this.num = i;
        this.num = Math.min(intValue, i);
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        if (!is2181() || this.num == 0) {
            return;
        }
        if (((sendImmediatelyEvent.getPacket904() instanceof PlayerMoveC2SPacket) && flag) || warp.isToggled()) {
            int i = this.num - 1;
            this.num = i;
            this.num = Math.max(0, i);
        }
    }

    public boolean is3092() {
        if (this.num <= 0 || !is2181() || !this.holeSnap.shift.getValue().booleanValue() || !minecraftClient.player.isOnGround() || BaritoneHelper_3.antiPhaseSearchHelper4_2.get2231() <= 1) {
            return false;
        }
        BaritoneHelper_3.inner.do2018(this.holeSnap, warp.boost.getValue().intValue());
        return true;
    }

    public boolean is2181() {
        return minecraftClient.player.getPos().squaredDistanceTo(minecraftClient.player.prevX, minecraftClient.player.prevY, minecraftClient.player.prevZ) > 0.0d;
    }
}
