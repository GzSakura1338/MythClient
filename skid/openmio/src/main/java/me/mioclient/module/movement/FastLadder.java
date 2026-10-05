package me.mioclient.module.movement;

import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/FastLadder.class */
public class FastLadder extends Module {
    public Setting<Float> speed;

    public FastLadder() {
        super("FastLadder", "Makes you move faster on ladders.", Category.MOVEMENT, new String[0]);
        PhaseESPHelper.do1351(this);
    }
}
