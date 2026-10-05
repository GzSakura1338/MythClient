package me.mioclient.module;

import java.nio.file.Path;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.MusicHelper;
import me.mioclient.PresetHelper;
import me.mioclient.feature.Size;
import net.minecraft.client.gui.DrawContext;
import org.apache.commons.lang3.SystemUtils;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Music.class */
public class Music extends me.mioclient.ModuleList {
    public static Path path;

    public Music() {
        super("Music", new String[0]);
        do3019(new Size(this));
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        if (SystemUtils.IS_OS_WINDOWS_10 || SystemUtils.OS_NAME.startsWith("Windows 11")) {
            return;
        }
        do495(false);
    }

    @Override // me.mioclient.ModuleList
    public void do364(DrawContext drawContext) {
        if (is2147()) {
            return;
        }
        FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, getString773(), 0.0f, 0.0f, getColor3018(this.moduleListSearchHelper4.get124()));
    }

    @Override // me.mioclient.ModuleList
    public float[] getFloatArray365() {
        return is2147() ? new float[]{0.0f, 0.0f} : new float[]{FontsSearchHelper4.fontsSearchHelper4.get1316(getString773()), FontsSearchHelper4.fontsSearchHelper4.get93()};
    }

    public static Path getPath2146() {
        if (path == null) {
            path = PresetHelper.path.resolve("music");
        }
        return path;
    }

    public String getString773() {
        return (!is3017() || MusicHelper.num == 4) ? MusicHelper.getString2782() : "HydrachFM - f3dot";
    }

    public boolean is2147() {
        return (is3017() || MusicHelper.num == 4) ? false : true;
    }
}
