package me.mioclient.module.misc;

import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/AutoReconnect.class */
public class AutoReconnect extends Module {
    public Setting<Float> delay;

    public AutoReconnect() {
        super("AutoReconnect", "Reconnects to the server automatically if you get kicked.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        setDrawn(false);
    }
}
