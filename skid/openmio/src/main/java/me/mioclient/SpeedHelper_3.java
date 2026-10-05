package me.mioclient;

import me.mioclient.event.MoveEvent;
import me.mioclient.module.movement.Speed;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpeedHelper_3.class */
public final class SpeedHelper_3 extends SpeedHelper {
    public SpeedHelper_3(Speed speed) {
        super(speed);
    }

    @Override // me.mioclient.SpeedHelper
    public void do242(MoveEvent moveEvent) {
        if (this.speed.is1815() || this.speed.is1814() || minecraftClient.player.isFallFlying() || this.speed.is2824()) {
            return;
        }
        HoleSnapSearchHelper4_3.getDoubleArray2507(moveEvent, this.speed.speed.getValue().floatValue());
    }
}
