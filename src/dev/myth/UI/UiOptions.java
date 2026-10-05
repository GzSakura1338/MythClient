package dev.myth.UI;

import dev.myth.UI.setting.Setting;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.List;

/** Mio control order; defaults match the user's saved ClickGUI preferences. */
public final class UiOptions {
    private final List<Setting<?>> settings = new ArrayList<>();
    public final Setting<Boolean> reset = add(Setting.bool("reset", "Reset", false).action());
    public final Setting<String> position = add(Setting.choice("position", "Position", "Custom", List.of("Custom", "Left", "Center", "Right"))
            .description("Aligns all category panels. Presets override custom positions; dragging returns to Custom."));
    public final Setting<String> prefix = add(Setting.text("prefix", "Prefix", "."));
    public final Setting<String> msaa = add(Setting.choice("msaa", "MSAA", "None", List.of("None", "X2", "X4", "X8")));
    public final Setting<Boolean> blur = add(Setting.bool("blur", "Blur", true).group());
    public final Setting<Float> blurStrength = add(Setting.decimal("blurStrength", "Strength", 6, 1, 20).visibleWhen(blur::isExpanded));
    public final Setting<Integer> blurFps = add(Setting.integer("blurFps", "FPS", 60, 1, 240).visibleWhen(blur::isExpanded));
    public final Setting<Boolean> descriptions = add(Setting.bool("descriptions", "Descriptions", true).group());
    public final Setting<Integer> delay = add(Setting.integer("delay", "Delay", 0, 0, 3000).unit("ms").visibleWhen(descriptions::isExpanded));
    public final Setting<Boolean> constantReset = add(Setting.bool("constantReset", "ConstantReset", false).visibleWhen(descriptions::isExpanded));
    public final Setting<Integer> buttonHeight = add(Setting.integer("buttonHeight", "ButtonHeight", 2, 0, 5));
    public final Setting<Integer> frameWidth = add(Setting.integer("frameWidth", "FrameWidth", 6, -10, 20));
    public final Setting<Integer> modulePadding = add(Setting.integer("modulePadding", "ModulePadding", 3, 1, 8));
    public final Setting<Float> animSpeed = add(Setting.decimal("animSpeed", "AnimSpeed", 1, .2f, 2.5f));
    public final Setting<Float> guiScale = add(Setting.decimal("guiScale", "GuiScale", 1, .1f, 2));
    public final Setting<Boolean> snow = add(Setting.bool("snow", "Snow", true).visibleWhen(UiOptions::isWinter));
    public final Setting<Boolean> elements = add(Setting.bool("elements", "Elements", true).group().action());
    public final Setting<Boolean> line = add(Setting.bool("line", "Line", true).group().visibleWhen(elements::isExpanded));
    public final Setting<Integer> lineColor = add(Setting.color("lineColor", "Color", 1941805567, true, false)
            .description("Border color. Enable Sync to follow the UI theme.")
            .visibleWhen(() -> elements.isExpanded() && line.isExpanded()));
    public final Setting<Boolean> binds = add(Setting.bool("binds", "Binds", false).visibleWhen(elements::isExpanded));
    public final Setting<Boolean> gear = add(Setting.bool("gear", "Gear", true).visibleWhen(elements::isExpanded));
    public final Setting<Boolean> windowShadow = add(Setting.bool("windowShadow", "WindowShadow", false).group().visibleWhen(elements::isExpanded));
    public final Setting<Integer> shadowColor = add(Setting.color("shadowColor", "ShadowColor", 0x46000000, true, true).visibleWhen(() -> elements.isExpanded() && windowShadow.isExpanded()));
    public final Setting<Float> shadowSize = add(Setting.decimal("shadowSize", "ShadowSize", 10, 1, 20).visibleWhen(() -> elements.isExpanded() && windowShadow.isExpanded()));
    public final Setting<Boolean> sounds = add(Setting.bool("sounds", "Sounds", true).group().action());
    public final Setting<Boolean> hover = add(Setting.bool("hover", "Hover", true).group().visibleWhen(sounds::isExpanded));
    public final Setting<String> hoverSound = add(Setting.sound("sound2", "Sound", "hover").visibleWhen(() -> sounds.isExpanded() && hover.isExpanded()));
    public final Setting<Float> hoverVolume = add(Setting.decimal("volume", "Volume", .5f, 0, 1).visibleWhen(() -> sounds.isExpanded() && hover.isExpanded()));
    public final Setting<Boolean> leftClick = add(Setting.bool("leftClick", "LeftClick", true).group().visibleWhen(sounds::isExpanded));
    public final Setting<String> clickSound = add(Setting.sound("sound3", "Sound", "click").visibleWhen(() -> sounds.isExpanded() && leftClick.isExpanded()));
    public final Setting<Float> clickVolume = add(Setting.decimal("volume3", "Volume", .5f, 0, 1).visibleWhen(() -> sounds.isExpanded() && leftClick.isExpanded()));
    public final Setting<Boolean> rightClick = add(Setting.bool("rightClick", "RightClick", true).group().visibleWhen(sounds::isExpanded));
    public final Setting<String> rightSound = add(Setting.sound("sound", "Sound", "rclick").visibleWhen(() -> sounds.isExpanded() && rightClick.isExpanded()));
    public final Setting<Float> rightVolume = add(Setting.decimal("volume2", "Volume", .5f, 0, 1).visibleWhen(() -> sounds.isExpanded() && rightClick.isExpanded()));
    public final Setting<Boolean> text = add(Setting.bool("text", "Text", false).group().action());
    public final Setting<Boolean> bounce = add(Setting.bool("bounce", "Bounce", true).visibleWhen(text::isExpanded));
    public final Setting<Integer> textColor = add(Setting.color("textColor", "TextColor", -396033, true, false).visibleWhen(text::isExpanded));
    public final Setting<Integer> enabledColor = add(Setting.color("enabledColor", "EnabledColor", -2242049, true, false).visibleWhen(text::isExpanded));
    public final Setting<Boolean> colors = add(Setting.bool("colors", "Colors", false).group().action());
    public final Setting<Integer> color = add(Setting.color("color", "Color", 1941805567, true, true).visibleWhen(colors::isExpanded));
    public final Setting<Integer> bgColor = add(Setting.color("bgColor", "BgColor", 1996619784, true, false).visibleWhen(colors::isExpanded));
    public final Setting<Integer> bgButton = add(Setting.color("bgButton", "BgButton", 12425727, true, true).visibleWhen(colors::isExpanded));
    public final Setting<Integer> bgEnabled = add(Setting.color("bgEnabled", "BgEnabled", 1941805567, true, true).visibleWhen(colors::isExpanded));

    public UiOptions() { colors.setExpanded(true); }

    public List<Setting<?>> getSettings() { return List.copyOf(settings); }
    public float width() { return 92 + frameWidth.get(); }
    public float rowHeight() { return 11 + buttonHeight.get(); }
    public int samples() { return msaa.get().equals("None") ? 1 : Integer.parseInt(msaa.get().substring(1)); }
    public int themeColor() { return color.color(color.get()); }
    public int color(Setting<Integer> setting) { return setting.color(themeColor()); }
    public static boolean isWinter() { int month = LocalDate.now().getMonthValue(); return month == 1 || month == 2 || month == 12; }
    private <T> Setting<T> add(Setting<T> setting) { settings.add(setting); return setting; }
}
