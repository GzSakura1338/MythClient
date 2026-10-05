package me.mioclient.module;

import me.mioclient.ArgumentTypeHelper;
import me.mioclient.CryptoHelper;
import me.mioclient.ModuleListMode;
import me.mioclient.ModuleListSearchHelper4_2;
import me.mioclient.SearchHelper4_8;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;
import net.minecraft.util.math.MathHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Direction.class */
public class Direction extends me.mioclient.ModuleList {
    public Direction() {
        super("Direction", "facing");
        do3019(new ModuleListSearchHelper4_2(this, new CryptoHelper(() -> {
            return Text.literal(getString1217());
        }, () -> {
            return true;
        })));
        getModuleListSearchHelper43020().do2952(ModuleListMode.BOTTOM_LEFT);
    }

    public String getString1217() {
        String string2921 = new ArgumentTypeHelper().getArgumentTypeHelper2906((int) MathHelper.wrapDegrees(minecraftClient.gameRenderer.getCamera().getYaw())).getArgumentTypeHelper2919(String.valueOf(Formatting.WHITE)).getString2921("\u0001 (\u0001, ");
        switch (SearchHelper4_8.get2488(minecraftClient.gameRenderer.getCamera().getYaw())) {
            case 0:
                return new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921).getString2921("South\u0001+Z)");
            case 1:
                return new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921).getString2921("SouthWest\u0001-X +Z)");
            case 2:
                return new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921).getString2921("West\u0001-X)");
            case 3:
                return new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921).getString2921("NorthWest\u0001-X -Z)");
            case 4:
                return new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921).getString2921("North\u0001-Z)");
            case 5:
                return new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921).getString2921("NorthEast\u0001+X -Z)");
            case 6:
                return new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921).getString2921("East\u0001+X)");
            case 7:
                return new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921).getString2921("SouthEast\u0001+X +Z)");
            default:
                return "Waiting...";
        }
    }
}
