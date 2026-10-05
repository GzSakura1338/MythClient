package today.starain.ui.clickgui.opal;

import today.starain.module.impl.client.ClickGuiModule;
import today.starain.ui.Theme;

/**
 * Design tokens for the Opal GUI.
 *
 * <p>Structural values are constants; the user-tunable ones ({@link #radius()},
 * {@link #panelWidth()}, opacities, tints and animation durations) read the
 * {@link ClickGuiModule} settings so they can be customized live. Accent colors read the
 * global {@link Theme}.</p>
 */
public final class OpalTheme {

    // ---- Surfaces (color bases; opacity is dynamic) ----
    public static final int HEADER_BG = 0xff0f0f0f;
    public static final int ROW_BG = 0xff1e1e2d;

    // ---- Text ----
    public static final int TEXT = 0xffffffff;
    public static final int TEXT_MUTED = 0xff9a9a9a;

    // ---- Fixed layout ----
    public static final float PANEL_SPACING = 10f;
    public static final float PANEL_TOP = 25f;
    public static final float ROW_HEIGHT = 20f;

    // ---- Fonts ----
    public static final float FONT_HEADER = 9f;
    public static final float FONT_ROW = 8f;
    public static final float FONT_BIND = 7f;
    public static final float ICON_HEADER = 10f;
    public static final float ICON_EXPAND = 12f;

    private OpalTheme() {
    }

    // ---- Tunable appearance (ClickGui module) ----
    public static float radius() {
        return ClickGuiModule.guiRadius();
    }

    public static float panelWidth() {
        return ClickGuiModule.guiPanelWidth();
    }

    public static float rowBgOpacity() {
        return ClickGuiModule.guiRowOpacity();
    }

    public static float headerBgOpacity() {
        return ClickGuiModule.guiHeaderOpacity();
    }

    public static float hoverOpacity() {
        return ClickGuiModule.guiHoverOpacity();
    }

    public static float toggleTint() {
        return ClickGuiModule.guiAccentTint();
    }

    // ---- Animation durations (0 = instant when animations are off) ----
    public static long animHover() {
        return ClickGuiModule.guiAnimations() ? (long) (150 / ClickGuiModule.guiAnimSpeed()) : 0L;
    }

    public static long animToggle() {
        return ClickGuiModule.guiAnimations() ? (long) (150 / ClickGuiModule.guiAnimSpeed()) : 0L;
    }

    public static long animExpand() {
        return ClickGuiModule.guiAnimations() ? (long) (125 / ClickGuiModule.guiAnimSpeed()) : 0L;
    }

    public static long animOpen(int index) {
        return ClickGuiModule.guiAnimations() ? (long) ((100 + index * 80L) / ClickGuiModule.guiAnimSpeed()) : 0L;
    }

    // ---- Accent (global theme or custom) ----
    public static int first() {
        if (!ClickGuiModule.guiSyncColor()) {
            return ClickGuiModule.guiAccentColor().getRGB();
        }
        return Theme.primary().getRGB();
    }

    public static int second() {
        if (!ClickGuiModule.guiSyncColor()) {
            return ClickGuiModule.guiAccentColor().getRGB();
        }
        return Theme.secondary().getRGB();
    }

    public static final float ACCENT_RATE = 0.1F;

    public static float accentRate() {
        return ClickGuiModule.guiCustomFlow() ? ClickGuiModule.guiFlowRate() : ACCENT_RATE;
    }

    public static int accent(int index) {
        return accent((double) index);
    }

    public static int accent(double index) {
        if (!ClickGuiModule.guiSyncColor()) {
            return ClickGuiModule.guiAccentColor().getRGB();
        }
        return Theme.accent(index, ClickGuiModule.guiCustomFlow() ? ClickGuiModule.guiFlowSpeed() : 0).getRGB();
    }
}
