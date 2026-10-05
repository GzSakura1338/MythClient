package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.setting.Setting;
import java.awt.Color;
import java.util.ArrayList;
import java.util.List;

/** Mio HUD colors and MixinTitleScreenMode None/Pulse/Raw. */
public final class HudColors {
    public final Setting<Boolean> colors = Setting.bool("colors", "Colors", true).group().action();
    public final Setting<Integer> color = Setting.color("color", "Color", 0xffbd99ff, false, false).visibleWhen(colors::isExpanded);
    public final Setting<String> fade = Setting.choice("fade", "Fade", "None", List.of("None", "Pulse", "Raw")).visibleWhen(colors::isExpanded);
    public final Setting<Integer> fadeOffset = Setting.integer("fadeOffset", "FadeOffset", 5, 1, 30)
            .visibleWhen(() -> colors.isExpanded() && (!fade.get().equals("None") || color.isRainbow()));
    public final Setting<Integer> secondColor = Setting.color("secondColor", "SecondColor", 0xff69568f, false, false)
            .visibleWhen(() -> colors.isExpanded() && fade.get().equals("Pulse"));
    public final Setting<Integer> colorAmount = Setting.integer("colorAmount", "ColorAmount", 5, 2, 9)
            .visibleWhen(() -> colors.isExpanded() && fade.get().equals("Raw"));
    public final Setting<Integer> fadeSpeed = Setting.integer("fadeSpeed", "FadeSpeed", 50, 20, 100)
            .visibleWhen(() -> colors.isExpanded() && fade.get().equals("Raw"));
    public final Setting<Boolean> mixColors = Setting.bool("mixColors", "MixColors", true)
            .visibleWhen(() -> colors.isExpanded() && fade.get().equals("Raw"));
    private final List<Setting<Integer>> palette = new ArrayList<>();
    private final List<Setting<?>> settings = new ArrayList<>(List.of(colors, color, fade, fadeOffset, secondColor, colorAmount, fadeSpeed, mixColors));

    public HudColors() {
        // Mio exposes up to nine colors; create them once so increasing ColorAmount
        // preserves both edited colors and their saved configuration.
        for (int i = 1; i <= 9; i++) {
            int index = i;
            var value = Setting.color("color" + i, "Color" + i, 0xffffffff, false, false)
                    .visibleWhen(() -> colors.isExpanded() && fade.get().equals("Raw") && index <= colorAmount.get());
            palette.add(value); settings.add(value);
        }
    }
    public List<Setting<?>> getSettings() { return List.copyOf(settings); }
    public int colorAt(float y, float fontHeight) { return colorAt(y, fontHeight, System.currentTimeMillis()); }
    int colorAt(float y, float fontHeight, long now) {
        int theme = Myth.UI_MANAGER.getThemeColor();
        float multiplier = 10 / (fontHeight + 2);
        int first = resolve(color, theme, now, 0);
        return switch (fade.get()) {
            case "Pulse" -> {
                double phase = ((now + y * fadeOffset.get() * multiplier) % 2000) / 2000;
                int second = resolve(secondColor, theme, now, 0);
                yield phase > .5 ? mix(second, first, (float) ((phase - .5) * 2)) : mix(first, second, (float) (phase * 2));
            }
            case "Raw" -> {
                int amount = colorAmount.get();
                float inverseOffset = 1f / fadeOffset.get();
                int time = (int) ((now / fadeSpeed.get()) % (amount * fadeOffset.get()) + y / multiplier);
                int index = Math.max(0, (int) Math.floor(time * inverseOffset));
                int a = resolve(palette.get(index % amount), theme, now, 0);
                yield mixColors.get() ? mix(a, resolve(palette.get((index + 1) % amount), theme, now, 0),
                        1 - (time * inverseOffset - index)) : a;
            }
            default -> color.isRainbow() && !color.isSync()
                    ? resolve(color, theme, now, (int) (y * fadeOffset.get() * multiplier)) : first;
        };
    }
    private static int resolve(Setting<Integer> setting, int theme, long now, int offset) {
        int argb = setting.get();
        if (setting.isSync()) return (argb & 0xff000000) | (theme & 0xffffff);
        if (!setting.isRainbow()) return argb;
        float[] hsv = Color.RGBtoHSB(argb >> 16 & 255, argb >> 8 & 255, argb & 255, null);
        float hue = (float) (Math.ceil((now + offset) / 20d) % 360) / 360;
        return (argb & 0xff000000) | (Color.HSBtoRGB(hue, hsv[1], hsv[2]) & 0xffffff);
    }
    private static int mix(int a, int b, float factor) {
        int result = 0;
        for (int shift = 0; shift <= 24; shift += 8)
            result |= (int) (((a >>> shift & 255) * factor) + ((b >>> shift & 255) * (1 - factor))) << shift;
        return result;
    }
}
