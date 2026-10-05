package today.starain.module.impl.client;

import io.github.humbleui.skija.Font;
import net.minecraft.client.renderer.texture.AbstractTexture;
import net.minecraft.client.renderer.texture.TextureAtlasSprite;
import net.minecraft.world.effect.MobEffectInstance;
import today.starain.Starain;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.ui.Theme;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Fonts;
import today.starain.value.impl.BooleanValue;

import java.awt.Color;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

public class EffectHUD extends ClientModule {

    private static final Color DURATION_COLOR = new Color(170, 170, 170);
    private static final Color SHADOW_COLOR = new Color(0, 0, 0, 170);
    private static final int ICON_SHADOW = 0xB0000000;
    private static final float ICON_GAP = 3.0f;

    private final BooleanValue syncColor = new BooleanValue("Sync Color", false, this);
    private final BooleanValue lowercase = new BooleanValue("Lowercase", false, this);
    private final BooleanValue icons = new BooleanValue("Icons", true, this);

    public EffectHUD() {
        setName("EffectHUD");
        setCategory(Category.CLIENT);
    }

    public static boolean isActive() {
        Starain starain = Starain.getInstance();
        if (starain == null || starain.getModuleManager() == null) {
            return false;
        }
        EffectHUD module = starain.getModuleManager().getModule(EffectHUD.class);
        return module != null && module.isEnabled();
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {
        if (mc.player == null || mc.level == null) {
            return;
        }
        List<MobEffectInstance> effects = new ArrayList<>();
        for (MobEffectInstance instance : mc.player.getActiveEffects()) {
            if (instance.showIcon()) {
                effects.add(instance);
            }
        }
        if (effects.isEmpty()) {
            return;
        }

        Font font = Fonts.getNormal(10.0f);
        effects.sort(Comparator.comparingDouble(instance -> -measure(instance, font)));

        float textHeight = Skia.getTextBounds("Ay", font).getHeight();
        float iconSize = icons.get() ? Math.max(8.0f, Math.round(textHeight + 2.0f)) : 0.0f;
        float rowHeight = Math.max(textHeight, iconSize);
        float lineHeight = rowHeight + 2.0f;

        float anchorX = mc.getWindow().getGuiScaledWidth() - 3.0f;
        float rowY = mc.getWindow().getGuiScaledHeight() - 3.0f - lineHeight;

        int index = 0;
        for (MobEffectInstance instance : effects) {
            String name = buildName(instance);
            String duration = buildDuration(instance);
            float nameWidth = Skia.getTextWidth(name, font);
            float durationWidth = Skia.getTextWidth(duration, font);
            float textWidth = nameWidth + 3.0f + durationWidth;

            Color color = syncColor.get()
                    ? Theme.accent(index * 0.6d)
                    : new Color(instance.getEffect().getColor());

            float textX = anchorX - textWidth;
            float textY = rowY + (rowHeight - textHeight) / 2.0f;
            float durationX = textX + nameWidth + 3.0f;
            Skia.drawTextShadow(name, textX, textY, SHADOW_COLOR, font);
            Skia.drawTextShadow(duration, durationX, textY, SHADOW_COLOR, font);
            Skia.drawText(name, textX, textY, color, font);
            Skia.drawText(duration, durationX, textY, DURATION_COLOR, font);

            if (icons.get()) {
                drawIcon(instance, Math.round(textX - ICON_GAP - iconSize),
                        Math.round(rowY + (rowHeight - iconSize) / 2.0f), iconSize);
            }

            rowY -= lineHeight;
            index++;
        }
    }

    private void drawIcon(MobEffectInstance instance, float x, float y, float size) {
        try {
            TextureAtlasSprite sprite = mc.getMobEffectTextures().get(instance.getEffect());
            if (sprite == null) {
                return;
            }
            AbstractTexture texture = mc.getTextureManager().getTexture(sprite.atlasLocation());
            if (texture == null) {
                return;
            }
            Skia.drawSpriteRegion(texture.getId(), x, y, size, size,
                    sprite.getU0(), sprite.getV0(), sprite.getU1(), sprite.getV1(), 1.0f, ICON_SHADOW);
        } catch (Throwable ignored) {
        }
    }

    private float measure(MobEffectInstance instance, Font font) {
        return Skia.getTextWidth(buildName(instance), font) + 3.0f
                + Skia.getTextWidth(buildDuration(instance), font);
    }

    private String buildName(MobEffectInstance instance) {
        String name = instance.getEffect().getDisplayName().getString();
        if (lowercase.get()) {
            name = name.toLowerCase(Locale.ROOT);
        }
        if (instance.getAmplifier() > 0) {
            name = name + " " + (instance.getAmplifier() + 1);
        }
        return name;
    }

    private String buildDuration(MobEffectInstance instance) {
        if (instance.isInfiniteDuration()) {
            return "**:**";
        }
        int seconds = instance.getDuration() / 20;
        int minutes = seconds / 60;
        seconds %= 60;
        int hours = minutes / 60;
        minutes %= 60;
        return hours > 0
                ? String.format(Locale.ROOT, "%d:%02d:%02d", hours, minutes, seconds)
                : String.format(Locale.ROOT, "%d:%02d", minutes, seconds);
    }
}
