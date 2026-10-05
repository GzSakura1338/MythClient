package me.mioclient.module.misc;

import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.util.Arrays;
import java.util.Comparator;
import java.util.function.Function;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/UnfocusedFPS.class */
public class UnfocusedFPS extends Module {
    public Setting<Integer> fps;

    public UnfocusedFPS() {
        super("UnfocusedFPS", "Lowers your FPS when Minecraft is in the background.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        setDrawn(false);
    }

    public int get2003() {
        GraphicsDevice[] screenDevices = GraphicsEnvironment.getLocalGraphicsEnvironment().getScreenDevices();
        Function<GraphicsDevice, Integer> function = graphicsDevice -> {
            return Integer.valueOf(graphicsDevice.getDisplayMode().getRefreshRate());
        };
        return ((Integer) Arrays.stream(screenDevices).max(Comparator.comparing(function)).map(function).orElse(60)).intValue();
    }
}
