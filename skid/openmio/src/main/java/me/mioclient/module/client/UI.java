package me.mioclient.module.client;

import java.awt.Color;
import java.time.LocalDate;
import java.util.concurrent.atomic.AtomicBoolean;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ChatFilterSearchHelper4_2;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FontsEvent;
import me.mioclient.FontsSearchHelper4_2;
import me.mioclient.MixinTitleScreenSearchHelper4;
import me.mioclient.Mode_10;
import me.mioclient.Mode_5;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchIdentifier;
import me.mioclient.api.Category;
import me.mioclient.api.Keybind;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.module.Module;
import net.minecraft.client.gui.screen.ChatScreen;
import net.minecraft.network.packet.s2c.common.DisconnectS2CPacket;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/client/UI.class */
public final class UI extends Module {
    public static UI uI;
    public final AtomicBoolean atomicBoolean;
    public Setting<Boolean> reset;
    public Setting<String> prefix;
    public Setting<Mode> msaa;
    public Setting<Boolean> descriptions;
    public Setting<Integer> delay;
    public Setting<Boolean> constantReset;
    public Setting<Integer> buttonHeight;
    public Setting<Integer> frameWidth;
    public Setting<Integer> modulePadding;
    public Setting<Float> animSpeed;
    public Setting<Float> guiScale;
    public Setting<Boolean> snow;
    public Setting<Boolean> elements;
    public Setting<Boolean> line;
    public Setting<Boolean> binds;
    public Setting<Boolean> gear;
    public Setting<Boolean> windowShadow;
    public Setting<Color> shadowColor;
    public Setting<Float> shadowSize;
    public Setting<Boolean> sounds;
    public Setting<Boolean> hover;
    public Setting<SearchIdentifier> sound2;
    public Setting<Float> volume;
    public Setting<Boolean> leftClick;
    public Setting<SearchIdentifier> sound3;
    public Setting<Float> volume3;
    public Setting<Boolean> rightClick;
    public Setting<SearchIdentifier> sound;
    public Setting<Float> volume2;
    public Setting<Boolean> text;
    public Setting<Boolean> bounce;
    public Setting<Color> textColor;
    public Setting<Color> enabledColor;
    public Setting<Boolean> colors;
    public Setting<Color> color;
    public Setting<Color> bgColor;
    public Setting<Color> bgButton;
    public Setting<Color> bgEnabled;
    public Setting<Color> tint;
    public Setting<Color> gradientColor;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/client/UI$Mode.class */
    public enum Mode implements EnumSettingHelper {
        NONE("None", 0),
        X2("X2", 2),
        X4("X4", 4),
        X8("X8", 8);

        public final String name;
        public final int num;

        Mode(String str, int i) {
            this.name = str;
            this.num = i;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public int get150() {
            return this.num;
        }
    }

    public UI() {
        super("UI", "Displays the client's click gui.", Category.CLIENT, new String[0]);
        this.atomicBoolean = new AtomicBoolean(false);
        PhaseESPHelper.do1351(this);
        baritoneHelper.do1797(UI.class);
        setDrawn(false);
        this.reset.do2339(() -> {
            if (this.reset.getValue().booleanValue()) {
                BaritoneHelper_3.getMixinTitleScreenSearchHelper42216().do2828();
                this.reset.do2333(false);
            }
        });
        this.frameWidth.do2339(() -> {
            this.atomicBoolean.set(true);
        });
        modifyKeybind(keybind -> {
            return keybind.getKeybind1941(344);
        });
        uI = this;
        this.sound2.do2329("HoverSound");
        this.volume.do2329("HoverVolume");
        this.sound3.do2329("LeftClickSound");
        this.volume3.do2329("LeftClickVolume");
        this.sound.do2329("RightClickSound");
        this.volume2.do2329("RightClickVolume");
        if (!Mode_10.WINTER.is2576(LocalDate.now().getMonthValue())) {
            this.snow.do2333(false);
            unregister((Setting<?>) this.snow);
        }
        this.prefix.do2339(() -> {
            if (this.prefix.getValue().equals(ChatFilterSearchHelper4_2.getString2982())) {
                return;
            }
            ChatFilterSearchHelper4_2.do2983(this.prefix.getValue());
        });
        this.prefix.do2353(true);
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        if (is1469() || (minecraftClient.currentScreen instanceof MixinTitleScreenSearchHelper4)) {
            return;
        }
        if (minecraftClient.currentScreen instanceof ChatScreen) {
            BaritoneHelper_3.tooltipsSearchHelper4_2.do164(() -> {
                minecraftClient.setScreen(BaritoneHelper_3.getMixinTitleScreenSearchHelper42216());
            }, 0);
        } else {
            minecraftClient.setScreen(BaritoneHelper_3.getMixinTitleScreenSearchHelper42216());
        }
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        if (minecraftClient.currentScreen instanceof MixinTitleScreenSearchHelper4) {
            BaritoneHelper_3.getMixinTitleScreenSearchHelper42216().do1714();
        }
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (minecraftClient.currentScreen instanceof FontsSearchHelper4_2) {
            return;
        }
        do496();
    }

    @Listen
    public void onEvent(FontsEvent fontsEvent) {
        if (GLFW.glfwGetMouseButton(minecraftClient.getWindow().getHandle(), 0) == 1 || !this.atomicBoolean.get()) {
            return;
        }
        BaritoneHelper_3.getMixinTitleScreenSearchHelper42216().do2828();
        this.reset.do2333(false);
        this.atomicBoolean.set(false);
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if (channelRead0Event.getPacket904() instanceof DisconnectS2CPacket) {
            disable();
        }
    }

    @Listen
    public static void onEvent2(FontsEvent fontsEvent) {
        if (minecraftClient.currentScreen instanceof FontsSearchHelper4_2) {
            return;
        }
        Mode_5.STANDARD.do935();
    }

    @Override // me.mioclient.module.Module
    public Keybind getKeybind() {
        Keybind keybind = super.getKeybind();
        if (keybind.getKeybindMode1946() != Keybind.KeybindMode.TOGGLE) {
            keybind = keybind.getKeybind1942(Keybind.KeybindMode.TOGGLE);
            setKeybind(keybind);
        }
        return keybind;
    }
}
