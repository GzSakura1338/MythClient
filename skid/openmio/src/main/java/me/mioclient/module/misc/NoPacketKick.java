package me.mioclient.module.misc;

import me.mioclient.ArgumentTypeHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/NoPacketKick.class */
public class NoPacketKick extends Module {
    public Setting<Boolean> message;

    public NoPacketKick() {
        super("NoPacketKick", "Cancels broken packets that may cause you getting kicked.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        setDrawn(false);
    }

    public Text getText1411(Throwable th) {
        return Text.translatable("disconnect.genericReason", new Object[]{new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(th)).getString2921("Internal Exception: \u0001")}).formatted(Formatting.RED);
    }
}
