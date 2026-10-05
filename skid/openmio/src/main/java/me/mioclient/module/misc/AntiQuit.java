package me.mioclient.module.misc;

import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/AntiQuit.class */
public class AntiQuit extends Module {
    public Setting<Boolean> disconnect;
    public Setting<Boolean> gameClose;

    public AntiQuit() {
        super("AntiQuit", "Prevents you from quitting the game/server accidentally.", Category.MISC, "antidisconnect");
        PhaseESPHelper.do1351(this);
    }
}
