package me.mioclient.module.misc;

import java.util.concurrent.ThreadLocalRandom;
import me.mioclient.EnumSettingHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/CustomDeathText.class */
public class CustomDeathText extends Module {
    public Setting<CustomDeathTextMode> mode;
    public Setting<String> message;
    public static final String[] stringArr = {"YOU USELESS SHITSTAIN", "GAME OVER MOTHERFUCKER! YOU SUCK!", "HOW THE HELL ARE YOU SO BAD AT THIS GAME?", "HOW DO YOU FUCK UP SO BADLY JESUS FUCKING CHRIST?", "YOU DIED! CONGRATS FUCKFACE", "OH MY GOD YOU SUCK! AUTO-UNINSTALLING", "HOLY FUCKING SHIT SERIOUSLY", "YOU FUCKING RETARD!", "YOU DENSE FUCK"};

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/CustomDeathText$CustomDeathTextMode.class */
    public enum CustomDeathTextMode implements EnumSettingHelper {
        CUSTOM("Custom"),
        RANDOM("Random");

        public final String name;

        CustomDeathTextMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public CustomDeathText() {
        super("CustomDeathText", "Displays a custom message on your death screen.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        setDrawn(false);
    }

    public Text getText327() {
        if (this.mode.getValue() == CustomDeathTextMode.CUSTOM) {
            return Text.of(this.message.getValue().trim());
        }
        return Text.literal(stringArr[ThreadLocalRandom.current().nextInt(stringArr.length)]);
    }
}
