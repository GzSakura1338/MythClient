package today.starain.module.impl.client;

import io.github.humbleui.skija.ClipMode;
import io.github.humbleui.skija.Font;
import io.github.humbleui.skija.Paint;
import io.github.humbleui.skija.Path;
import io.github.humbleui.types.RRect;
import io.github.humbleui.types.Rect;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.phys.EntityHitResult;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.combat.KillAura;
import today.starain.ui.Design;
import today.starain.ui.clickgui.opal.OpalClickGUI;
import today.starain.utils.animation.SmoothAnimationTimer;
import today.starain.utils.render.ProjectionUtils;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.vector.Vector2f;
import today.starain.utils.render.skia.font.Fonts;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;

import today.starain.ui.Theme;

import java.awt.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class TargetHUD extends ClientModule {
    private final NumberValue x = new NumberValue("X", 0, 1000, 300, 1, this);
    private final NumberValue y = new NumberValue("y", 0, 1000, 300, 1, this);
    private final BooleanValue syncColor = new BooleanValue("Sync Color", false, this);
    private final BooleanValue glow = new BooleanValue("Glow", false, this);
    private final BooleanValue followPlayer = new BooleanValue("Follow Player", false, this);
    private final BooleanValue multiTarget = new BooleanValue("Multi Target", false, this);
    private final BooleanValue hitParticles = new BooleanValue("Hit Particles", false, this);
    private final NumberValue particleCount = new NumberValue("Particle Count", 4, 48, 20, 1, this, hitParticles::get);
    private final ModeValue side = new ModeValue("Side", "Right", new String[]{"Right", "Left", "Auto"}, this, followPlayer::get);
    private final NumberValue sideOffset = new NumberValue("Side Offset", 0, 80, 16, 1, this, followPlayer::get);
    private final NumberValue heightRatio = new NumberValue("Height", 0.0, 1.5, 0.8, 0.05, this, followPlayer::get);
    private final Map<Integer, SmoothAnimationTimer> healTimers = new HashMap<>();
    private final Map<Integer, SmoothAnimationTimer> hurtTimers = new HashMap<>();
    private final SmoothAnimationTimer heal = new SmoothAnimationTimer(20.0F);
    private final SmoothAnimationTimer alpha = new SmoothAnimationTimer(255.0F);
    private final SmoothAnimationTimer hurtAlpha = new SmoothAnimationTimer(0.0F);
    private final Map<Integer, HitEffect> hitEffects = new HashMap<>();
    private final Map<Integer, RollText> healthRolls = new HashMap<>();
    private long lastFrameNano;
    private float frameDelta;
    private float previewTimer;
    private AbstractClientPlayer target;

    private static final class Particle {
        float offsetX;
        float offsetY;
        float velocityX;
        float velocityY;
        float life;
        float maxLife;
        float size;
        float phase;
    }

    private static final class RollSlot {
        char shown;
        char incoming;
        float progress = 1.0f;
        int direction = 1;
    }

    private static final class RollText {
        final List<RollSlot> slots = new ArrayList<>();
        float value = Float.NaN;
        long lastSeen;
    }

    private static final class HitEffect {
        final List<Particle> particles = new ArrayList<>();
        int lastHurtTime;
        float lastHealth = Float.NaN;
    }

    public TargetHUD() {
        setName("TargetHUD");
        setCategory(Category.CLIENT);
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {
        if (mc.player == null) return;
        float partialTick = event.getPartialTick();
        updateFrameDelta();

        if (mc.screen instanceof OpalClickGUI) {
            alpha.update(true);
            alpha.value = 255.0f;
            target = mc.player;
            heal.update(true);
            hurtAlpha.update(true);
            previewTimer += frameDelta;
            if (hitParticles.get() && previewTimer >= 1.2f) {
                previewTimer = 0.0f;
                spawnBurst(effectFor(mc.player));
            }
            drawHud(mc.player, this.x.get().floatValue(), this.y.get().floatValue(), "Fixed",
                    alpha.value, heal.value, hurtAlpha.value);
            return;
        }

        KillAura killAura = Starain.getInstance().getModuleManager().getModule(KillAura.class);
        if (multiTarget.get() && killAura != null && killAura.isEnabled() && killAura.isSwitchMode()
                && KillAura.targets != null && !KillAura.targets.isEmpty()) {
            healTimers.keySet().removeIf(id -> mc.level == null || mc.level.getEntity(id) == null);
            hurtTimers.keySet().removeIf(id -> mc.level == null || mc.level.getEntity(id) == null);
            for (Entity entity : KillAura.targets) {
                if (entity instanceof AbstractClientPlayer player) {
                    drawTracked(player, partialTick);
                }
            }
            return;
        }

        alpha.update(true);
        if (alpha.value < 1.0f || target == null) return;
        heal.update(true);
        hurtAlpha.update(true);

        if (followPlayer.get()) {
            Vector2f screen = projectSide(target, partialTick);
            if (screen == null) return;
            drawHud(target, screen.x, screen.y, anchorFor(screen), alpha.value, heal.value, hurtAlpha.value);
            return;
        }
        drawHud(target, this.x.get().floatValue(), this.y.get().floatValue(), "Fixed",
                alpha.value, heal.value, hurtAlpha.value);
    }

    private void drawTracked(AbstractClientPlayer player, float partialTick) {
        SmoothAnimationTimer healTimer = healTimers.computeIfAbsent(player.getId(), id -> new SmoothAnimationTimer(player.getHealth()));
        SmoothAnimationTimer hurtTimer = hurtTimers.computeIfAbsent(player.getId(), id -> new SmoothAnimationTimer(0.0F));
        healTimer.target = player.getHealth();
        hurtTimer.target = player.hurtTime > 0 ? 120.0f : 0.0f;
        healTimer.update(true);
        hurtTimer.update(true);

        if (followPlayer.get()) {
            Vector2f screen = projectSide(player, partialTick);
            if (screen == null) return;
            drawHud(player, screen.x, screen.y, anchorFor(screen), 255.0f, healTimer.value, hurtTimer.value);
            return;
        }
        int index = KillAura.targets.indexOf(player);
        drawHud(player, this.x.get().floatValue(), this.y.get().floatValue() + index * 45.0f, "Fixed",
                255.0f, healTimer.value, hurtTimer.value);
    }

    private Vector2f projectSide(AbstractClientPlayer player, float partialTick) {
        double px = Mth.lerp(partialTick, player.xo, player.getX());
        double py = Mth.lerp(partialTick, player.yo, player.getY()) + player.getBbHeight() * heightRatio.get().doubleValue();
        double pz = Mth.lerp(partialTick, player.zo, player.getZ());
        Vector2f screen = ProjectionUtils.project(px, py, pz, partialTick);
        if (screen.x == Float.MAX_VALUE || !Float.isFinite(screen.x) || !Float.isFinite(screen.y)) {
            return null;
        }
        return screen;
    }

    private String anchorFor(Vector2f screen) {
        if (side.isMode("Auto")) {
            return screen.x < mc.getWindow().getGuiScaledWidth() / 2.0f ? "Right" : "Left";
        }
        return side.get();
    }

    private void drawHud(AbstractClientPlayer target, float baseX, float baseY, String anchor,
                         float alphaValue, float healValue, float hurtValue) {
        float width = 125.0f;
        float height = 40.0f;
        Font miSans = Fonts.getNormal(10.0f);
        Font iconFill = Fonts.getIconFill(10.0f);
        float heartIconWidth = iconFill.measureTextWidth("\ue87d");
        String name = target.getName().getString();
        float nameWidth = Skia.getTextWidth(name, miSans);
        float healBarWidth = (nameWidth + 45.0f > width) ? nameWidth : 78.0f;
        if (nameWidth + 45.0f > width) width = nameWidth + 45.0f;

        float offset = sideOffset.get().floatValue();
        float x;
        float y;
        switch (anchor) {
            case "Right" -> {
                x = baseX + offset;
                y = baseY - height / 2.0f;
            }
            case "Left" -> {
                x = baseX - offset - width;
                y = baseY - height / 2.0f;
            }
            default -> {
                x = baseX;
                y = baseY;
            }
        }

        if (glow.get()) {
            today.starain.module.impl.render.Glow.queueHud(x, y, width, height, 6.0f, alphaValue / 255.0f, true);
        }

        try (Paint layerPaint = new Paint()) {
            layerPaint.setAlpha((int) alphaValue);
            Skia.getCanvas().saveLayer(Rect.makeXYWH(x - 10, y - 10, width + 20, height + 20), layerPaint);
            if (alphaValue >= 75f) {
                Skia.drawShadow(x, y, width, height, 6.0f);
                Skia.drawBlur(x, y, width, height, 6.0f);
            }
            Skia.drawRoundedRect(x, y, width, height, 6.0f, Design.BACKGROUND_COLOR);
            Skia.drawPlayerHead(target, x + 5.0f, y + 5.0f, 30.0f, 30.0f, 4.0f);
            if (hurtValue > 1.0f) {
                Skia.drawRoundedRect(x + 5.0f, y + 5.0f, 30.0f, 30.0f, 4.0f, new Color(255, 80, 80, (int) hurtValue));
            }
            Path path = new Path();
            path.addRRect(RRect.makeXYWH(x + 40.0f, y + 30.5f, healBarWidth, 5.0f, 3.0f));
            Skia.save();
            Skia.getCanvas().clipPath(path, ClipMode.INTERSECT, true);
            Skia.drawRect(x + 40.0f, y + 30.5f, healBarWidth, 5.0f, new Color(0, 0, 0, 50));
            float healthWidth = (healBarWidth / 20.0f) * healValue;
            if (syncColor.get()) {
                Skia.drawThemeGradientRect(x + 40.0f, y + 30.5f, healthWidth, 5.0f,
                        x + 40.0f, x + 40.0f + healBarWidth, Math.max(2, Math.round(healBarWidth / 6.0f)));
            } else {
                Skia.drawRect(x + 40.0f, y + 30.5f, healthWidth, 5.0f, new Color(200, 200, 200));
            }
            Skia.restore();
            String healthText = String.format("%.2f", target.getHealth());
            if (syncColor.get()) {
                Skia.drawGradientText(name, x + 40.0f, y + 4.5f, miSans);
                Skia.drawGradientText("\ue87d", x + 40.0f, y + 18f, iconFill);
            } else {
                Skia.drawText(name, x + 40.0f, y + 4.5f, Design.TEXT_COLOR, miSans);
                Skia.drawText("\ue87d", x + 40.0f, y + 18f, new Color(200, 200, 200), iconFill);
            }
            drawRolling(target.getId(), healthText, target.getHealth(),
                    x + 40.0f + heartIconWidth, y + 17.5f, miSans);
            path.close();
            Skia.restore();
        }
        if (hitParticles.get()) {
            renderParticles(target, x + 20.0f, y + 20.0f, alphaValue / 255.0f);
        }
    }

    private void drawRolling(int id, String text, float value, float x, float yTop, Font font) {
        if (healthRolls.size() > 32) healthRolls.clear();
        RollText roll = healthRolls.computeIfAbsent(id, key -> new RollText());
        long now = System.nanoTime();
        boolean fresh = Float.isNaN(roll.value) || now - roll.lastSeen > 300_000_000L;
        roll.lastSeen = now;
        if (fresh) {
            snap(roll, text);
            roll.value = value;
        } else if (Math.abs(value - roll.value) > 0.001f) {
            sync(roll, text, value >= roll.value ? 1 : -1);
            roll.value = value;
        }

        Rect bounds = font.measureText(text);
        float penX = x - bounds.getLeft();
        float baseline = yTop - bounds.getTop();
        float travel = bounds.getHeight() + 2.0f;

        Skia.save();
        Skia.clipRect(x - 1.0f, yTop - 1.0f, font.measureTextWidth(text) + 3.0f, bounds.getHeight() + 3.0f);
        for (int i = 0; i < roll.slots.size(); i++) {
            RollSlot slot = roll.slots.get(i);
            slot.progress = Math.min(1.0f, slot.progress + frameDelta * 4.5f);
            String incoming = String.valueOf(slot.incoming);
            Color color = syncColor.get() ? Theme.accent(-i) : new Color(200, 200, 200);
            if (slot.progress >= 1.0f) {
                Skia.drawBaselineText(incoming, penX, baseline, color.getRGB() | 0xFF000000, font);
            } else {
                float eased = 1.0f - (float) Math.pow(1.0f - slot.progress, 3.0d);
                int outAlpha = (int) (255.0f * (1.0f - eased));
                int inAlpha = (int) (255.0f * eased);
                Skia.drawBaselineText(String.valueOf(slot.shown), penX,
                        baseline - slot.direction * travel * eased,
                        (outAlpha << 24) | (color.getRGB() & 0xFFFFFF), font);
                Skia.drawBaselineText(incoming, penX,
                        baseline + slot.direction * travel * (1.0f - eased),
                        (inAlpha << 24) | (color.getRGB() & 0xFFFFFF), font);
            }
            penX += font.measureTextWidth(incoming);
        }
        Skia.restore();
    }

    private static void snap(RollText roll, String text) {
        roll.slots.clear();
        for (int i = 0; i < text.length(); i++) {
            RollSlot slot = new RollSlot();
            slot.shown = text.charAt(i);
            slot.incoming = text.charAt(i);
            slot.progress = 1.0f;
            roll.slots.add(slot);
        }
    }

    private static void sync(RollText roll, String text, int direction) {
        while (roll.slots.size() > text.length()) {
            roll.slots.remove(roll.slots.size() - 1);
        }
        while (roll.slots.size() < text.length()) {
            RollSlot slot = new RollSlot();
            slot.shown = ' ';
            slot.incoming = text.charAt(roll.slots.size());
            slot.progress = 0.0f;
            slot.direction = direction;
            roll.slots.add(slot);
        }
        for (int i = 0; i < text.length(); i++) {
            RollSlot slot = roll.slots.get(i);
            char next = text.charAt(i);
            if (slot.incoming != next) {
                slot.shown = slot.incoming;
                slot.incoming = next;
                slot.progress = 0.0f;
                slot.direction = direction;
            }
        }
    }

    private void updateFrameDelta() {
        long now = System.nanoTime();
        frameDelta = lastFrameNano == 0L ? 0.0f : Math.min(0.05f, (now - lastFrameNano) / 1.0E9f);
        lastFrameNano = now;
    }

    private HitEffect effectFor(AbstractClientPlayer player) {
        if (hitEffects.size() > 32) hitEffects.clear();
        return hitEffects.computeIfAbsent(player.getId(), id -> new HitEffect());
    }

    private void renderParticles(AbstractClientPlayer player, float centerX, float centerY, float alphaScale) {
        HitEffect effect = effectFor(player);
        if (effect.particles.isEmpty() && player.hurtTime <= effect.lastHurtTime) {
            effect.lastHurtTime = player.hurtTime;
            effect.lastHealth = player.getHealth();
            return;
        }
        float health = player.getHealth();
        boolean hurt = player.hurtTime > effect.lastHurtTime
                || (!Float.isNaN(effect.lastHealth) && health < effect.lastHealth - 0.01f);
        effect.lastHurtTime = player.hurtTime;
        effect.lastHealth = health;
        if (hurt) spawnBurst(effect);

        double phaseSpan = 360.0 / themeSpread();
        Iterator<Particle> iterator = effect.particles.iterator();
        while (iterator.hasNext()) {
            Particle particle = iterator.next();
            particle.life -= frameDelta;
            if (particle.life <= 0.0f) {
                iterator.remove();
                continue;
            }
            particle.velocityY += 110.0f * frameDelta;
            particle.offsetX += particle.velocityX * frameDelta;
            particle.offsetY += particle.velocityY * frameDelta;

            float progress = particle.life / particle.maxLife;
            int particleAlpha = (int) (255.0f * progress * alphaScale);
            if (particleAlpha <= 2) continue;
            Color base = syncColor.get() ? Theme.accent(particle.phase * phaseSpan) : new Color(255, 160, 160);
            float drawX = centerX + particle.offsetX;
            float drawY = centerY + particle.offsetY;
            float radius = particle.size * (0.5f + 0.5f * progress);
            Skia.drawCircle(drawX, drawY, radius * 2.6f,
                    new Color(base.getRed(), base.getGreen(), base.getBlue(), particleAlpha / 6));
            Skia.drawCircle(drawX, drawY, radius,
                    new Color(base.getRed(), base.getGreen(), base.getBlue(), particleAlpha));
        }
    }

    private int themeSpread() {
        HUD hud = Starain.getInstance().getModuleManager().getModule(HUD.class);
        return hud == null ? 1 : Math.max(1, hud.getThemeSpread());
    }

    private void spawnBurst(HitEffect effect) {
        int count = particleCount.get().intValue();
        if (effect.particles.size() > count * 6) return;
        for (int i = 0; i < count; i++) {
            Particle particle = new Particle();
            double angle = i / (double) count * Math.PI * 2.0d + (Math.random() - 0.5d) * 0.4d;
            double speed = 48.0d + Math.random() * 62.0d;
            particle.velocityX = (float) (Math.cos(angle) * speed);
            particle.velocityY = (float) (Math.sin(angle) * speed) - 26.0f;
            particle.maxLife = 0.55f + (float) Math.random() * 0.4f;
            particle.life = particle.maxLife;
            particle.size = 1.7f + (float) Math.random() * 1.7f;
            particle.phase = i / (float) count;
            effect.particles.add(particle);
        }
    }

    @EventTarget
    public void onPreTick(GameTickEvent event) {
        if (mc.player == null || event.getType() != EventType.PRE) return;
        KillAura module = Starain.getInstance().getModuleManager().getModule(KillAura.class);
        if (mc.hitResult instanceof EntityHitResult entityHitResult && entityHitResult.getEntity() instanceof AbstractClientPlayer player) {
            alpha.target = 255.0f;
            target = player;
            heal.target = player.getHealth();
            hurtAlpha.target = player.hurtTime > 0 ? 120.0f : 0.0f;
        } else if (module.isEnabled() && KillAura.getTarget() != null && KillAura.getTarget() instanceof AbstractClientPlayer player) {
            alpha.target = 255.0f;
            target = player;
            heal.target = player.getHealth();
            hurtAlpha.target = player.hurtTime > 0 ? 120.0f : 0.0f;
        } else {
            alpha.target = 0.0f;
            hurtAlpha.target = 0.0f;
        }
    }
}
