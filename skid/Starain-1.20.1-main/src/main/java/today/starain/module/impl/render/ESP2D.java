package today.starain.module.impl.render;

import io.github.humbleui.skija.Paint;
import io.github.humbleui.skija.Shader;
import io.github.humbleui.types.Point;
import io.github.humbleui.types.Rect;
import net.minecraft.client.model.HumanoidModel;
import net.minecraft.client.model.geom.ModelPart;
import net.minecraft.client.renderer.entity.EntityRenderer;
import net.minecraft.client.renderer.entity.LivingEntityRenderer;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.Pose;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.phys.AABB;
import org.joml.Vector3f;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.misc.Target;
import today.starain.ui.Theme;
import today.starain.utils.render.ProjectionUtils;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.vector.Vector2f;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ColorValue;
import today.starain.value.impl.NumberValue;

import java.awt.Color;

public class ESP2D extends ClientModule {

    private static final Color OUTLINE_COLOR = new Color(0, 0, 0, 255);

    private final BooleanValue syncColor = new BooleanValue("Sync Color", true, this);
    private final ColorValue color = new ColorValue("Color", new Color(91, 207, 250), this, () -> !syncColor.get());
    private final NumberValue spread = new NumberValue("Color Spread", 0.0, 200.0, 50.0, 1.0, this, syncColor::get);
    private final NumberValue lineWidth = new NumberValue("Line Width", 0.25, 3.0, 0.5, 0.05, this);
    private final BooleanValue box = new BooleanValue("Box", true, this);
    private final BooleanValue outline = new BooleanValue("Outline", true, this, box::get);
    private final BooleanValue healthBar = new BooleanValue("Health Bar", false, this);
    private final BooleanValue skeleton = new BooleanValue("Skeleton", false, this);
    private final NumberValue skeletonWidth = new NumberValue("Skeleton Width", 0.1, 2.0, 0.5, 0.05, this, skeleton::get);
    private final BooleanValue useTarget = new BooleanValue("Use Target", false, this);
    private final BooleanValue glow = new BooleanValue("Glow", false, this);

    public ESP2D() {
        setName("2DESP");
        setCategory(Category.RENDER);
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {
        if (mc.level == null || mc.player == null) {
            return;
        }
        float partialTick = event.getPartialTick();

        if (useTarget.get()) {
            for (Entity entity : mc.level.entitiesForRendering()) {
                if (Target.isTarget(entity)) {
                    drawEntity(entity, partialTick);
                }
            }
            return;
        }

        for (Player player : mc.level.players()) {
            if (player == mc.player || !player.isAlive() || player.isSpectator() || player.isInvisible()) {
                continue;
            }
            drawEntity(player, partialTick);
        }
    }

    private void drawEntity(Entity entity, float partialTick) {
        float[] bounds = project(entity, partialTick);
        if (bounds == null) {
            return;
        }

        float minX = bounds[0];
        float minY = bounds[1];
        float maxX = bounds[2];
        float maxY = bounds[3];
        float boxWidth = maxX - minX;
        float boxHeight = maxY - minY;
        if (boxWidth <= 0.0F || boxHeight <= 0.0F) {
            return;
        }

        float line = lineWidth.get().floatValue();
        float thick = line * 3.0F;

        if (skeleton.get()) {
            drawSkeleton(entity, partialTick, skeletonWidth.get().floatValue());
        }

        if (healthBar.get() && entity instanceof LivingEntity living) {
            float ratio = Mth.clamp(living.getHealth() / Math.max(living.getMaxHealth(), 1.0F), 0.0F, 1.0F);
            float barWidth = line * 2.0F;
            float barX = minX - thick - barWidth - line;
            Skia.drawRect(barX - line, minY - line, barWidth + line * 2.0F, boxHeight + line * 2.0F, OUTLINE_COLOR);
            float filled = boxHeight * ratio;
            Skia.drawRect(barX, minY + boxHeight - filled, barWidth, filled, Color.getHSBColor(ratio / 3.0F, 1.0F, 1.0F));
        }

        if (!box.get()) {
            if (glow.get()) {
                Glow.queueHud(minX, minY, boxWidth, boxHeight, 0.0F);
            }
            return;
        }

        if (outline.get()) {
            Skia.drawRect(minX, minY, boxWidth, thick, OUTLINE_COLOR);
            Skia.drawRect(minX, minY, thick, boxHeight + thick, OUTLINE_COLOR);
            Skia.drawRect(maxX, minY, thick, boxHeight + thick, OUTLINE_COLOR);
            Skia.drawRect(minX, maxY, boxWidth, thick, OUTLINE_COLOR);
        }

        Color head = accent(0.0F);
        Color tail = accent(boxHeight);

        gradient(minX + line, minY + line, boxWidth, line, head, tail, false);
        gradient(minX + line, minY + line, line, boxHeight + line, head, tail, true);
        gradient(maxX + line, minY + line, line, boxHeight + line, tail, head, true);
        gradient(minX + line, maxY + line, boxWidth, line, tail, head, false);

        if (glow.get()) {
            Glow.queueHud(minX, minY, boxWidth, boxHeight, 0.0F);
        }
    }

    @SuppressWarnings("unchecked")
    private void drawSkeleton(Entity entity, float partialTick, float width) {
        if (!(entity instanceof LivingEntity living)) {
            return;
        }
        EntityRenderer<?> renderer = mc.getEntityRenderDispatcher().getRenderer(entity);
        if (!(renderer instanceof LivingEntityRenderer<?, ?> livingRenderer)) {
            return;
        }
        if (!(livingRenderer.getModel() instanceof HumanoidModel<?> raw)) {
            return;
        }
        if (living.deathTime > 0 || living.isAutoSpinAttack() || living.isFallFlying()
                || living.hasPose(Pose.SLEEPING) || living.getSwimAmount(partialTick) > 0.0F) {
            return;
        }
        HumanoidModel<LivingEntity> model = (HumanoidModel<LivingEntity>) raw;

        float bodyRot = Mth.rotLerp(partialTick, living.yBodyRotO, living.yBodyRot);
        float headRot = Mth.rotLerp(partialTick, living.yHeadRotO, living.yHeadRot);
        float headPitch = Mth.lerp(partialTick, living.xRotO, living.getXRot());
        float limbSwing = living.walkAnimation.position(partialTick);
        float limbSwingAmount = living.walkAnimation.speed(partialTick);

        model.young = living.isBaby();
        model.riding = living.isPassenger();
        model.attackTime = living.getAttackAnim(partialTick);
        model.prepareMobModel(living, limbSwing, limbSwingAmount, partialTick);
        model.setupAnim(living, limbSwing, limbSwingAmount, living.tickCount + partialTick,
                headRot - bodyRot, headPitch);
        double rad = Math.toRadians(180.0D - bodyRot);
        double cos = Math.cos(rad);
        double sin = Math.sin(rad);
        double ex = Mth.lerp(partialTick, entity.xo, entity.getX());
        double ey = Mth.lerp(partialTick, entity.yo, entity.getY());
        double ez = Mth.lerp(partialTick, entity.zo, entity.getZ());
        double modelScale = living instanceof Player ? 0.9375D : 1.0D;
        double[] origin = {cos, sin, ex, ey, ez, modelScale};
        Color tint = accent(0.0F);

        Vector2f neck = pivot(model.body, origin, partialTick);
        Vector2f headTop = extend(model.head, -7.5F, origin, partialTick);
        Vector2f hip = extend(model.body, 12.0F, origin, partialTick);
        Vector2f rightShoulder = pivot(model.rightArm, origin, partialTick);
        Vector2f leftShoulder = pivot(model.leftArm, origin, partialTick);
        Vector2f rightHand = extend(model.rightArm, 9.5F, origin, partialTick);
        Vector2f leftHand = extend(model.leftArm, 9.5F, origin, partialTick);
        Vector2f rightHip = pivot(model.rightLeg, origin, partialTick);
        Vector2f leftHip = pivot(model.leftLeg, origin, partialTick);
        Vector2f rightFoot = extend(model.rightLeg, 11.5F, origin, partialTick);
        Vector2f leftFoot = extend(model.leftLeg, 11.5F, origin, partialTick);

        bone(neck, headTop, width, tint);
        bone(neck, hip, width, tint);
        bone(rightShoulder, leftShoulder, width, tint);
        bone(rightShoulder, rightHand, width, tint);
        bone(leftShoulder, leftHand, width, tint);
        bone(rightHip, leftHip, width, tint);
        bone(rightHip, rightFoot, width, tint);
        bone(leftHip, leftFoot, width, tint);
    }

    private void bone(Vector2f from, Vector2f to, float width, Color color) {
        if (from == null || to == null) {
            return;
        }
        Skia.drawLine(from.x, from.y, to.x, to.y, width, color);
    }

    private Vector2f pivot(ModelPart part, double[] origin, float partialTick) {
        return modelToScreen(part.x, part.y, part.z, origin, partialTick);
    }

    private Vector2f extend(ModelPart part, float length, double[] origin, float partialTick) {
        Vector3f offset = new Vector3f(0.0F, length, 0.0F);
        offset.rotateX(part.xRot);
        offset.rotateY(part.yRot);
        offset.rotateZ(part.zRot);
        return modelToScreen(part.x + offset.x, part.y + offset.y, part.z + offset.z, origin, partialTick);
    }

    private Vector2f modelToScreen(double mx, double my, double mz, double[] origin, float partialTick) {
        double scale = origin[5];
        double lx = -scale * mx / 16.0D;
        double ly = scale * (1.501D - my / 16.0D);
        double lz = scale * mz / 16.0D;
        double cos = origin[0];
        double sin = origin[1];
        double wx = origin[2] + lx * cos + lz * sin;
        double wy = origin[3] + ly;
        double wz = origin[4] - lx * sin + lz * cos;
        Vector2f screen = ProjectionUtils.project(wx, wy, wz, partialTick);
        if (screen.x == Float.MAX_VALUE || !Float.isFinite(screen.x) || !Float.isFinite(screen.y)) {
            return null;
        }
        return screen;
    }

    private Color accent(float offset) {
        if (!syncColor.get()) {
            return color.get();
        }
        return Theme.accent(-offset * (spread.get().floatValue() / 100.0F));
    }

    private void gradient(float x, float y, float width, float height, Color from, Color to, boolean vertical) {
        if (width <= 0.0F || height <= 0.0F) {
            return;
        }
        int start = io.github.humbleui.skija.Color.makeARGB(from.getAlpha(), from.getRed(), from.getGreen(), from.getBlue());
        int end = io.github.humbleui.skija.Color.makeARGB(to.getAlpha(), to.getRed(), to.getGreen(), to.getBlue());
        try (Paint paint = new Paint()) {
            paint.setAntiAlias(true);
            Point p1 = new Point(x, y);
            Point p2 = vertical ? new Point(x, y + height) : new Point(x + width, y);
            try (Shader shader = Shader.makeLinearGradient(p1, p2, new int[]{start, end})) {
                paint.setShader(shader);
                Skia.getCanvas().drawRect(Rect.makeXYWH(x, y, width, height), paint);
            }
        }
    }

    private float[] project(Entity entity, float partialTick) {
        double px = Mth.lerp(partialTick, entity.xo, entity.getX());
        double py = Mth.lerp(partialTick, entity.yo, entity.getY());
        double pz = Mth.lerp(partialTick, entity.zo, entity.getZ());
        double halfWidth = (entity.getBbWidth() + 0.2D) / 2.0D;
        double height = entity.getBbHeight() + (entity.isCrouching() ? -0.3D : 0.2D) + 0.05D;
        AABB box = new AABB(px - halfWidth, py, pz - halfWidth, px + halfWidth, py + height, pz + halfWidth);

        float minX = Float.MAX_VALUE;
        float minY = Float.MAX_VALUE;
        float maxX = -Float.MAX_VALUE;
        float maxY = -Float.MAX_VALUE;
        boolean any = false;

        for (int i = 0; i < 8; i++) {
            double cx = (i & 1) == 0 ? box.minX : box.maxX;
            double cy = (i & 2) == 0 ? box.minY : box.maxY;
            double cz = (i & 4) == 0 ? box.minZ : box.maxZ;
            Vector2f screen = ProjectionUtils.project(cx, cy, cz, partialTick);
            if (screen.x == Float.MAX_VALUE || !Float.isFinite(screen.x) || !Float.isFinite(screen.y)) {
                continue;
            }
            any = true;
            minX = Math.min(minX, screen.x);
            minY = Math.min(minY, screen.y);
            maxX = Math.max(maxX, screen.x);
            maxY = Math.max(maxY, screen.y);
        }

        return any ? new float[]{minX, minY, maxX, maxY} : null;
    }
}
