package me.mioclient.module.client;

import java.util.concurrent.atomic.AtomicBoolean;
import me.mioclient.FontsEvent;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.FontsSearchHelper4_2;
import me.mioclient.Mode_8;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/client/Fonts.class */
public class Fonts extends Module {
    public static Fonts fonts;
    public final AtomicBoolean atomicBoolean;
    public static final Runnable runnable = () -> {
        if (minecraftClient.player != null || (minecraftClient.currentScreen instanceof FontsSearchHelper4_2)) {
            FontsSearchHelper4.fontsSearchHelper4.do1687(FontsSearchHelper4.fontsSearchHelper4.getAdvanceGlyph1688());
        }
    };
    public Setting<String> font;
    public Setting<Mode_8> style;
    public Setting<Boolean> antiAlias;
    public Setting<Boolean> chat;
    public Setting<Integer> size;
    public Setting<Integer> shift;
    public Setting<Float> shadow;
    public Setting<Integer> translate;

    public Fonts() {
        super("Fonts", "Manages the client's font renderer.", Category.CLIENT, new String[0]);
        this.atomicBoolean = new AtomicBoolean(false);
        PhaseESPHelper.do1351(this);
        this.style.do2339(runnable);
        this.antiAlias.do2339(runnable);
        this.size.do2339(() -> {
            this.atomicBoolean.set(true);
        });
        this.font.do2339(runnable);
        fonts = this;
        setDrawn(false);
    }

    @Listen
    public void onEvent(FontsEvent fontsEvent) {
        if (GLFW.glfwGetMouseButton(minecraftClient.getWindow().getHandle(), 0) == 1 || !this.atomicBoolean.get()) {
            return;
        }
        FontsSearchHelper4.fontsSearchHelper4.do1687(FontsSearchHelper4.fontsSearchHelper4.getAdvanceGlyph1688());
        this.atomicBoolean.set(false);
    }
}
