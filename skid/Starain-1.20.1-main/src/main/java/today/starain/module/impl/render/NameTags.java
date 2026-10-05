package today.starain.module.impl.render;

import io.github.humbleui.skija.Font;
import io.github.humbleui.types.Rect;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.player.Player;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.client.HUD;
import today.starain.module.impl.misc.Teams;
import today.starain.utils.player.FriendManager;
import today.starain.utils.render.ProjectionUtils;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Fonts;
import today.starain.utils.vector.Vector2f;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.NumberValue;

import java.awt.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;

public class NameTags extends ClientModule {
    private static final Color BACKGROUND_COLOR = new Color(0, 0, 0, 100);
    private static final Color HEALTH_TEXT_COLOR = new Color(200, 200, 200);
    private static final Color HEART_ICON_COLOR = new Color(255, 85, 85);
    private static final String HEART_ICON = String.valueOf((char) 0xE87D);
    private final Map<Entity, Vector2f> entityPositions = new ConcurrentHashMap<>();
    private final List<String> statusParts = new ArrayList<>();

    private final NumberValue scale = new NumberValue("Scale", 0.1, 1.0, 0.3, 0.01, this);
    private final BooleanValue mcf = new BooleanValue("MCF", true, this);
    private final BooleanValue blur = new BooleanValue("Blur", true, this);
    private final BooleanValue bloom = new BooleanValue("Bloom", true, this);

    public NameTags() {
        setName("NameTags");
        setCategory(Category.RENDER);
    }

    @EventTarget
    public void onRender(RenderSkiaEvent e) {
        try {
            this.updatePositions(e.getPartialTick());
        } catch (Exception ignored) {
        }
        this.renderModern();
    }

    private void renderModern() {
        boolean useShadow = HUD.isShadowEnabled();
        boolean useZetaBlur = HUD.isBlurEnabled();

        for (Entry<Entity, Vector2f> entry : this.entityPositions.entrySet()) {
            if (entry.getKey() != mc.player && entry.getKey() instanceof Player living) {
                float hp = living.getHealth();
                if (hp > 20.0F) {
                    living.setHealth(20.0F);
                }

                Vector2f position = entry.getValue();

                float scaleValue = this.scale.get().floatValue();
                float padding = 6.0f * scaleValue;
                float radius = 6.0f * scaleValue;
                float spacing = 4.0f * scaleValue;
                float iconGap = 3.0f * scaleValue;

                this.statusParts.clear();
                if (Teams.isSameTeam(living)) this.statusParts.add("§aTeam");
                if (this.mcf.get() && FriendManager.isFriend(living)) this.statusParts.add("§aFriend");

                String statusText = String.join(" ", this.statusParts);
                String nameText = living.getName().getString();
                String healthText = String.valueOf(Math.round(hp));
                if (living.getAbsorptionAmount() > 0.0F) {
                    healthText += "+" + Math.round(living.getAbsorptionAmount());
                }

                float baseFontSize = 14f * scaleValue;
                Font font = Fonts.getNormal(baseFontSize);
                Font iconFont = Fonts.getIconFill(baseFontSize);

                float statusWidth = statusText.isEmpty() ? 0 : Skia.getTextWidth(statusText, font) + (padding * 2);
                float nameWidth = Skia.getTextWidth(nameText, font) + (padding * 2);
                float heartWidth = iconFont.measureTextWidth(HEART_ICON);
                float healthWidth = Skia.getTextWidth(healthText, font) + heartWidth + (padding * 2) + iconGap;

                float totalWidth = nameWidth + healthWidth + spacing;
                if (statusWidth > 0) totalWidth += statusWidth + spacing;
                float height = baseFontSize + (padding * 1.5f);

                float startX = position.x - totalWidth / 2.0f;
                float startY = position.y - height / 2.0f;

                float currentX = startX;

                if (!statusText.isEmpty()) {
                    this.renderSegment(currentX, startY, statusWidth, height, radius, padding, iconGap, statusText, font, Color.WHITE, null, null, useShadow, useZetaBlur);
                    currentX += statusWidth + spacing;
                }

                this.renderSegment(currentX, startY, nameWidth, height, radius, padding, iconGap, nameText, font, Color.WHITE, null, null, useShadow, useZetaBlur);
                currentX += nameWidth + spacing;

                this.renderSegment(currentX, startY, healthWidth, height, radius, padding, iconGap, healthText, font, HEALTH_TEXT_COLOR, HEART_ICON, iconFont, useShadow, useZetaBlur);
            }
        }
    }

    private void renderSegment(float x, float y, float width, float height, float radius, float padding, float iconGap, String text, Font font, Color textColor, String icon, Font iconFont, boolean useShadow, boolean useZetaBlur) {
        if (this.bloom.get() && useShadow) {
            Skia.drawShadow(x, y, width, height, radius);
        }
        if (this.blur.get()) {
            if (useZetaBlur) {
                Skia.drawGuiRoundedBlur(x, y, width, height, radius);
            } else {
                Skia.drawBlur(x, y, width, height, radius);
            }
        }
        Skia.drawRoundedRect(x, y, width, height, radius, BACKGROUND_COLOR);

        float contentX = x + padding;

        Rect textBounds = font.measureText(text.replaceAll("§.", ""));
        float textY = y + (height - textBounds.getHeight()) / 2.0f;

        Skia.drawTextPlain(text, contentX, textY, textColor, font);

        if (icon != null && iconFont != null) {
            float textWidth = Skia.getTextWidth(text, font);
            Rect iconBounds = iconFont.measureText(icon);
            float iconY = y + (height - iconBounds.getHeight()) / 2.0f;
            Skia.drawTextPlain(icon, contentX + textWidth + iconGap, iconY, HEART_ICON_COLOR, iconFont);
        }
    }

    private void updatePositions(float renderPartialTicks) {
        this.entityPositions.clear();

        for (Entity entity : mc.level.entitiesForRendering()) {
            if (entity instanceof Player && !entity.getName().getString().startsWith("CIT-")) {
                double x = Mth.lerp(renderPartialTicks, entity.xo, entity.getX());
                double y = Mth.lerp(renderPartialTicks, entity.yo, entity.getY()) + (double) entity.getBbHeight() + 0.5;
                double z = Mth.lerp(renderPartialTicks, entity.zo, entity.getZ());
                Vector2f vector = ProjectionUtils.project(x, y, z, renderPartialTicks);
                // 摄像机背后 / 无效投影时 ProjectionUtils 返回 Float.MAX_VALUE。若不过滤，
                // 后面 drawShadow/drawBlur 会把 MAX_VALUE 坐标塞进共享的 shadowPath/blurPath，
                // 下一帧 MixinGui clipPath(DIFFERENCE) 被这块巨型矩形击穿，导致整个 shadow/blur 消失
                // （并刷 GL_INVALID_OPERATION）。这里直接跳过屏幕外的玩家。
                if (!Float.isFinite(vector.x) || !Float.isFinite(vector.y)
                        || vector.x == Float.MAX_VALUE || vector.y == Float.MAX_VALUE) {
                    continue;
                }
                vector.y = vector.y - 2.0F;
                this.entityPositions.put(entity, vector);
            }
        }
    }
}
