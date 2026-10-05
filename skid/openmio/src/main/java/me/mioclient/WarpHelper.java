package me.mioclient;

import java.util.LinkedList;
import java.util.Queue;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.module.movement.Warp;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.c2s.common.CommonPongC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/WarpHelper.class */
public class WarpHelper implements SearchHelper_4 {
    public final Queue<Packet<?>> queue = new LinkedList();
    public final Warp warp;

    public WarpHelper(Warp warp) {
        baritoneHelper.do1796(this);
        this.warp = warp;
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (this.warp.is2181() || this.warp.mode.getValue() != Warp.WarpMode.PLAIN) {
            return;
        }
        this.warp.num = Math.min(this.warp.charge.getValue().intValue(), this.warp.num + this.warp.chargeSpeed.getValue().intValue());
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        if (is1469()) {
            return;
        }
        if (this.warp.isToggled() && this.warp.mode.getValue() == Warp.WarpMode.ALTERNATIVE) {
            Packet<?> packet904 = sendImmediatelyEvent.getPacket904();
            if (packet904 instanceof CommonPongC2SPacket) {
                Packet<?> packet = (CommonPongC2SPacket) packet904;
                if (!this.warp.is2180() && this.queue.size() < this.warp.charge.getValue().intValue()) {
                    sendImmediatelyEvent.do1162();
                    this.queue.add(packet);
                }
                if (!this.queue.isEmpty()) {
                    sendImmediatelyEvent.do1162();
                }
            }
        }
        if (this.warp.mode.getValue() == Warp.WarpMode.PLAIN && (sendImmediatelyEvent.getPacket904() instanceof PlayerMoveC2SPacket) && this.warp.is2181() && this.warp.num != 0) {
            if (this.warp.isToggled() || this.warp.recharge.getValue() != WarpHelperMode.HOLD) {
                this.warp.num = Math.max(0, this.warp.num - 1);
            }
        }
    }
}
