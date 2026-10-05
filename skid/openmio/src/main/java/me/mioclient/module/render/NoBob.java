package me.mioclient.module.render;

import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/render/NoBob.class */
public class NoBob extends Module {
    public Setting<Float> multiplier;

    public NoBob() {
        super("NoBob", "Modifies the bobbing animation.", Category.RENDER, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (!is1469() && this.multiplier.getValue().floatValue() <= 0.0f) {
            minecraftClient.player.horizontalSpeed = 0.0f;
        }
    }
}
