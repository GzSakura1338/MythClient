package me.mioclient.module;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.CryptoHelper;
import me.mioclient.EnumSetting;
import me.mioclient.EnumSettingHelper;
import me.mioclient.ModuleListSearchHelper4_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.StringSetting;
import me.mioclient.api.Setting;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Welcomer.class */
public class Welcomer extends me.mioclient.ModuleList {
    public Setting<WelcomerMode> setting;
    public Setting<String> setting2;

    static enum WelcomerMode implements EnumSettingHelper {
        welcomerMode("Name") {
            @Override public Text getText963(Welcomer welcomer) {
                return Text.literal("Hello " + SearchHelper_4.minecraftClient.player.getName().getString() + " :')");
            }
        },
        welcomerMode2("UID") {
            @Override public Text getText963(Welcomer welcomer) {
                return Text.literal("Hello uid" + BaritoneHelper_3.welcomerHelper.get2811() + " :')");
            }
        },
        welcomerMode3("Custom") {
            @Override public Text getText963(Welcomer welcomer) {
                return Text.literal(welcomer.setting2.getValue());
            }
        };

        public final String name;
        WelcomerMode(String str2) { this.name = str2; }
        @Override public String getName() { return this.name; }
        public Text getText963(Welcomer welcomer) { return null; }
    }

    public Welcomer() {
        super("Welcomer", new String[0]);
        this.setting = add(new EnumSetting("Mode", WelcomerMode.welcomerMode));
        this.setting2 = add(new StringSetting("Text", "Welcome to Mio.", str -> {
            return this.setting.getValue() == WelcomerMode.welcomerMode3;
        }));
        do3019(new ModuleListSearchHelper4_2(this, new CryptoHelper(() -> {
            return this.setting.getValue().getText963(this);
        }, () -> {
            return true;
        })));
    }
}
