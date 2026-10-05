package me.mioclient.module.misc;

import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.TimeUnit;
import me.mioclient.EnumSettingHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.client.option.GameOptions;
import net.minecraft.entity.player.PlayerModelPart;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/SkinFlicker.class */
public class SkinFlicker extends Module {
    public static final PlayerModelPart[] playerModelPartArr = {PlayerModelPart.LEFT_SLEEVE, PlayerModelPart.JACKET, PlayerModelPart.HAT, PlayerModelPart.LEFT_PANTS_LEG, PlayerModelPart.RIGHT_PANTS_LEG, PlayerModelPart.RIGHT_SLEEVE};
    public static final PlayerModelPart[] playerModelPartArr2 = {PlayerModelPart.HAT, PlayerModelPart.JACKET, PlayerModelPart.LEFT_SLEEVE, PlayerModelPart.RIGHT_SLEEVE, PlayerModelPart.LEFT_PANTS_LEG, PlayerModelPart.RIGHT_PANTS_LEG};
    public Setting<SkinFlickerMode> mode;
    public Setting<Boolean> cape;
    public Setting<Float> delay;
    public final Map<PlayerModelPart, Boolean> map;
    public final int num;
    public final Stopwatch stopwatch;
    public int num2;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/SkinFlicker$SkinFlickerMode.class */
    public enum SkinFlickerMode implements EnumSettingHelper {
        FULL("Full"),
        VERTICAL("Vertical"),
        HORIZONTAL("Horizontal"),
        RANDOM("Random");

        public final String name;

        SkinFlickerMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public SkinFlicker() {
        super("SkinFlicker", "Flicks your skin parts.", Category.MISC, "skinblinker");
        PhaseESPHelper.do1351(this);
        this.map = new HashMap();
        this.num = PlayerModelPart.values().length;
        this.stopwatch = new Stopwatch();
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        for (PlayerModelPart playerModelPart : PlayerModelPart.values()) {
            this.map.put(playerModelPart, Boolean.valueOf(minecraftClient.options.isPlayerModelPartEnabled(playerModelPart)));
        }
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        Map<PlayerModelPart, Boolean> map = this.map;
        GameOptions gameOptions = minecraftClient.options;
        Objects.requireNonNull(gameOptions);
        map.forEach((v1, v2) -> {
            gameOptions.togglePlayerModelPart(v1, v2);
        });
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (this.stopwatch.is418(this.delay.getValue().floatValue(), TimeUnit.SECONDS)) {
            switch (this.mode.getValue()) {
                case FULL:
                    for (PlayerModelPart playerModelPart : PlayerModelPart.values()) {
                        if (is2685(playerModelPart)) {
                            do2684(playerModelPart);
                        }
                    }
                    break;
                case VERTICAL:
                case HORIZONTAL:
                    PlayerModelPart playerModelPart2 = (this.mode.getValue() == SkinFlickerMode.HORIZONTAL ? playerModelPartArr : playerModelPartArr2)[this.num2 % 6];
                    if (is2685(playerModelPart2)) {
                        do2684(playerModelPart2);
                        this.num2 = (this.num2 + 1) % 6;
                        break;
                    }
                    break;
                case RANDOM:
                    PlayerModelPart playerModelPart3 = PlayerModelPart.values()[ThreadLocalRandom.current().nextInt(this.num)];
                    if (is2685(playerModelPart3)) {
                        do2684(playerModelPart3);
                        break;
                    }
                    break;
            }
            this.stopwatch.reset();
        }
    }

    public void do2684(PlayerModelPart playerModelPart) {
        minecraftClient.options.togglePlayerModelPart(playerModelPart, !minecraftClient.options.isPlayerModelPartEnabled(playerModelPart));
    }

    public boolean is2685(PlayerModelPart playerModelPart) {
        return this.cape.getValue().booleanValue() || playerModelPart != PlayerModelPart.CAPE;
    }
}
