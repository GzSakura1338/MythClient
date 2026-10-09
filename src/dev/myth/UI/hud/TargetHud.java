package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import dev.myth.model.combat.KillAura;
import imgui.ImGui;
import java.awt.Color;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;

/**
 * The panel for whoever KillAura is on, in the same visual language as the other panel HUDs: square
 * corners and the one-pixel outside border MioDraw draws, Map's blur and panel colours, and themed
 * text through HudColors. Nothing here rounds or anti-aliases an edge, by house style.
 *
 * <p>Layout is a framed portrait column on the left and a text column on the right, three rows
 * deep - name with the distance right aligned against it, the health bar, then the health readout.
 */
public final class TargetHud extends HudElement {
    private static final float PAD = 5, GAP = 6, PORTRAIT_W = 30, PORTRAIT_H = 36;
    private static final float BAR_H = 4, NAME_Y = 4, BAR_Y = 19, READOUT_Y = 27;
    /** The trailing segment holds for this long before it starts catching up to the real health. */
    private static final long GHOST_HOLD = 400;

    public final Setting<Float> size = addSetting(Setting.decimal("size", "Size", 1, .5f, 2));
    public final Setting<Boolean> blur = addSetting(Setting.bool("blur", "Blur", true));
    public final Setting<Integer> background = addSetting(Setting.color("background", "Background", 0x320a0a0a, true, false));
    public final Setting<Integer> outline = addSetting(Setting.color("outline", "Outline", 0x640a0a0a, true, false));
    public final Setting<Boolean> portrait = addSetting(Setting.bool("portrait", "Portrait", true));
    public final Setting<Integer> healthColor = addSetting(Setting.color("healthColor", "HealthColor", 0xff55ff55, false, false).group());
    public final Setting<Boolean> healthFade = addSetting(Setting.bool("healthFade", "LowHealthFade", true)
            .description("Walks the bar through yellow to red as the target runs out of health.")
            .visibleWhen(() -> healthColor.isExpanded()));
    public final Setting<Boolean> ghost = addSetting(Setting.bool("ghost", "DamageTrail", true)
            .description("Leaves the previous health behind the bar for a moment after a hit.")
            .visibleWhen(() -> healthColor.isExpanded()));
    public final Setting<Boolean> maximum = addSetting(Setting.bool("maximum", "ShowMax", true)
            .description("Writes the max health after the current one, dimmed."));
    public final Setting<Boolean> distance = addSetting(Setting.bool("distance", "Distance", true));

    private float fade;
    private float shownHealth = -1, ghostHealth = -1;
    private long ghostAt;
    private LivingEntity shown;
    private float measuredWidth = 118;

    public TargetHud() { super("target_hud", "TargetHud"); }

    @Override public String getDescription() { return "Shows the target KillAura is on, with health and distance."; }
    @Override public boolean wantsBlur() { return blur.get(); }

    /** Editing has no combat target, so the panel previews with the local player like Welcomer does. */
    private LivingEntity target(boolean editing) {
        Entity aura = KillAura.getTarget();
        if (aura instanceof LivingEntity living && living.isAlive()) return living;
        return editing ? Minecraft.getInstance().player : null;
    }

    private float textLeft() { return PAD + (portrait.get() ? PORTRAIT_W + GAP : 0); }

    @Override protected float measureWidth(FontManager fonts) { return measuredWidth * size.get(); }
    @Override protected float measureHeight(FontManager fonts) { return (PORTRAIT_H + PAD * 2) * size.get(); }

    @Override protected void render(RenderContext context) {
        LivingEntity target = target(context.editing());
        float step = Math.clamp(ImGui.getIO().getDeltaTime() / .2f, 0, 1);
        fade += ((target != null ? 1 : 0) - fade) * step;
        if (target != null && target != shown) { shown = target; shownHealth = ghostHealth = -1; }
        float alpha = fade * context.opacity();
        if (shown == null || alpha <= .01f) return;

        FontManager fonts = context.fonts();
        float scale = size.get(), x = context.x(), y = context.y(), w = context.width(), h = context.height();
        float max = Math.max(1, shown.getMaxHealth());
        float actual = Math.clamp(shown.getHealth(), 0, max);
        if (shownHealth < 0) { shownHealth = ghostHealth = actual; }
        shownHealth += (actual - shownHealth) * step;
        if (shownHealth > ghostHealth) ghostHealth = shownHealth;
        else if (System.currentTimeMillis() - ghostAt > GHOST_HOLD) ghostHealth += (shownHealth - ghostHealth) * step * .5f;
        if (actual < ghostHealth - .01f && shownHealth > actual + .01f) ghostAt = System.currentTimeMillis();

        String name = shown.getName().getString();
        String current = "%.1f".formatted(shownHealth);
        String limit = maximum.get() ? " / " + "%.0f".formatted(max) : "";
        String range = distance.get() && Minecraft.getInstance().player != null
                ? "%.1fm".formatted(Minecraft.getInstance().player.distanceTo(shown)) : "";

        // Re-measured here so layout picks it up next frame, the way ModuleList sizes its rows.
        float column = Math.max(Math.max(fonts.width(name) + 8 + fonts.width(range),
                fonts.width(current) + fonts.width(limit)), 70);
        measuredWidth = textLeft() + column + PAD;

        if (blur.get()) Myth.UI_MANAGER.drawHudBlur(context.draw(), x - 1, y - 1, w + 2, h + 2, alpha);
        HudDraw.rect(context.draw(), x, y, w, h, background.color(HudDraw.theme()), alpha);
        HudDraw.outline(context.draw(), x - 1, y - 1, w + 1, h + 1, outline.color(HudDraw.theme()), alpha);

        if (portrait.get()) {
            float px = x + PAD * scale, py = y + PAD * scale;
            float pw = PORTRAIT_W * scale, ph = PORTRAIT_H * scale;
            // An inset of its own so the model reads as framed instead of floating on the panel.
            HudDraw.rect(context.draw(), px, py, pw, ph, 0x28000000, alpha);
            HudDraw.outline(context.draw(), px - 1, py - 1, pw + 1, ph + 1, outline.color(HudDraw.theme()), alpha * .7f);
            context.draw().pushClipRect(px, py, px + pw, py + ph, true);
            // 16 keeps the same body framing the 50x80 player panel gets at 35.
            Myth.HUD_MANAGER.assets.entity(context.draw(), shown, px, py,
                    (int) PORTRAIT_W, (int) PORTRAIT_H, 16, scale, alpha);
            context.draw().popClipRect();
        }

        float textX = x + textLeft() * scale, right = x + w - PAD * scale;
        float nameY = y + NAME_Y * scale;
        fonts.draw(context.draw(), name, textX, nameY,
                Myth.HUD_MANAGER.colors.colorAt(nameY, fonts.height()), alpha, scale);
        if (!range.isEmpty())
            fonts.draw(context.draw(), range, right - fonts.width(range) * scale, nameY,
                    Myth.HUD_MANAGER.colors.colorAt(nameY, fonts.height()), alpha * .6f, scale);

        float barY = y + BAR_Y * scale, barW = right - textX;
        HudDraw.rect(context.draw(), textX, barY, barW, BAR_H * scale, 0x55000000, alpha);
        if (ghost.get() && ghostHealth > shownHealth)
            HudDraw.rect(context.draw(), textX, barY, barW * Math.clamp(ghostHealth / max, 0, 1), BAR_H * scale,
                    0x60ffffff, alpha);
        float fraction = Math.clamp(shownHealth / max, 0, 1);
        HudDraw.rect(context.draw(), textX, barY, barW * fraction, BAR_H * scale, barColor(fraction), alpha);
        HudDraw.outline(context.draw(), textX - 1, barY - 1, barW + 1, BAR_H * scale + 1,
                outline.color(HudDraw.theme()), alpha);

        float readoutY = y + READOUT_Y * scale;
        int readoutColor = Myth.HUD_MANAGER.colors.colorAt(readoutY, fonts.height());
        fonts.draw(context.draw(), current, textX, readoutY, readoutColor, alpha, scale);
        if (!limit.isEmpty())
            fonts.draw(context.draw(), limit, textX + fonts.width(current) * scale, readoutY, readoutColor, alpha * .5f, scale);
    }

    /**
     * Through hue rather than RGB. Lerping the configured green straight to red passes through a
     * muddy olive at half health; rotating the hue down to 0 goes green, yellow, orange, red, which
     * is what reads as a health bar.
     */
    private int barColor(float fraction) {
        int base = healthColor.color(HudDraw.theme());
        if (!healthFade.get()) return base;
        float[] hsb = Color.RGBtoHSB(base >> 16 & 255, base >> 8 & 255, base & 255, null);
        return (base & 0xff000000) | (Color.HSBtoRGB(hsb[0] * fraction, hsb[1], hsb[2]) & 0xffffff);
    }
}
