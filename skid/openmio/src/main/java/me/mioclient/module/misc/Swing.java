package me.mioclient.module.misc;

import me.mioclient.EnumSettingHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.util.Hand;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/Swing.class */
public class Swing extends Module {
    public Setting<ScaffoldHelperMode> hand;
    public Setting<MixinLivingEntityMode> type;
    public Setting<Double> speed;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/Swing$MixinLivingEntityMode.class */
    public enum MixinLivingEntityMode implements EnumSettingHelper {
        VANILLA("Vanilla"),
        ONE_EIGHT("1.8"),
        ONE_TWELVE("1.12");

        public final String name;

        MixinLivingEntityMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/Swing$ScaffoldHelperMode.class */
    public static enum ScaffoldHelperMode implements EnumSettingHelper {
        scaffoldHelperMode("Mainhand") {
            @Override // me.mioclient.module.misc.Swing.ScaffoldHelperMode
            public Hand getHand2084() {
                return Hand.MAIN_HAND;
            }
        },
        scaffoldHelperMode2("Offhand") {
            @Override // me.mioclient.module.misc.Swing.ScaffoldHelperMode
            public Hand getHand2084() {
                return Hand.OFF_HAND;
            }
        },
        PACKET("Packet"),
        VANILLA("Vanilla");

        public final String name;

        ScaffoldHelperMode(String str2) {
            this.name = str2;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public Hand getHand2084() {
            return null;
        }
    }

    public Swing() {
        super("Swing", "Changes your swinging hand.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
    }
}
