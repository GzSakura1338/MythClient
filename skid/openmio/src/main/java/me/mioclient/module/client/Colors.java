package me.mioclient.module.client;

import java.awt.Color;
import me.mioclient.KeybindModule;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchMode;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/client/Colors.class */
public class Colors extends KeybindModule {
    public Setting<Color> friendColor;
    public Setting<Color> enemyColor;
    public Setting<Color> themeColor;
    public Setting<SearchMode> scheme;

    public Colors() {
        super("Colors", "Manages the client's color system.", Category.CLIENT, new String[0]);
        PhaseESPHelper.do1351(this);
    }
}
