package me.mioclient.module.player;

import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/NameProtect.class */
public class NameProtect extends Module {
    public Setting<String> name;
    public Setting<Boolean> skin;
    public Setting<Boolean> slim;

    public NameProtect() {
        super("NameProtect", "Hides your nickname from the curious people.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
    }
}
