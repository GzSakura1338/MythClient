package me.mioclient.module;

import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.BooleanSetting;
import me.mioclient.CryptoHelper;
import me.mioclient.ModuleListMode;
import me.mioclient.ModuleListSearchHelper4_2;
import me.mioclient.StringSetting;
import me.mioclient.api.Setting;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Watermark.class */
public class Watermark extends me.mioclient.ModuleList {
    public Setting<Boolean> setting;
    public Setting<String> setting2;

    public Watermark() {
        super("Watermark", new String[0]);
        this.setting = add(new BooleanSetting("VersionColor", true));
        this.setting2 = add(new StringSetting("Text", "Mio"));
        do3019(new ModuleListSearchHelper4_2(this, new CryptoHelper(() -> {
            return Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(getString1922()).getArgumentTypeHelper2919(String.valueOf(this.setting.getValue().booleanValue() ? Formatting.WHITE : "")).getArgumentTypeHelper2919(this.setting2.getValue().trim()).getString2921("\u0001\u0001\u0001"));
        }, () -> {
            return true;
        })));
        getModuleListSearchHelper43020().do2952(ModuleListMode.TOP_LEFT);
        do495(true);
    }

    public String getString1922() {
        return BaritoneHelper_3.welcomerHelper.get2811() == 62 ? " v2.1.7+" : " v2.1.7";
    }
}
