package today.starain.module.impl.render;

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.BufferBuilder;
import com.mojang.blaze3d.vertex.DefaultVertexFormat;
import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.blaze3d.vertex.Tesselator;
import com.mojang.blaze3d.vertex.VertexFormat;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.phys.EntityHitResult;
import net.minecraft.world.phys.Vec3;
import org.joml.Matrix4f;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.event.impl.RenderWorldEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.combat.KillAura;
import today.starain.ui.Theme;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ColorValue;
import today.starain.value.impl.NumberValue;

import java.awt.Color;

public class TargetESP extends ClientModule {
    private static final int SEGMENTS = 64;

    private final BooleanValue syncColor = new BooleanValue("Sync Color", true, this);
    private final ColorValue color = new ColorValue("Color", new Color(255, 255, 255, 233), this, () -> !syncColor.get());
    private final NumberValue radius = new NumberValue("Radius", 0.1, 2.0, 0.75, 0.05, this);
    private final NumberValue alphaFactor = new NumberValue("Alpha Factor", 0.0, 2.0, 1.0, 0.05, this);
    private final BooleanValue through = new BooleanValue("Through Walls", true, this);

    private LivingEntity target;

    public TargetESP() {
        setName("TargetESP");
        setCategory(Category.RENDER);
    }

    @Override
    public void onDisable() {
        target = null;
    }

    @EventTarget
    public void onTick(GameTickEvent event) {
        if (event.getType() != EventType.PRE) return;
        if (mc.player == null || mc.level == null) {
            target = null;
            return;
        }

        if (mc.hitResult instanceof EntityHitResult entityHitResult
                && entityHitResult.getEntity() instanceof LivingEntity living
                && living != mc.player) {
            target = living;
            return;
        }

        KillAura killAura = Starain.getInstance().getModuleManager().getModule(KillAura.class);
        if (killAura != null && killAura.isEnabled()) {
            Entity auraTarget = KillAura.getTarget();
            if (auraTarget instanceof LivingEntity living && living != mc.player) {
                target = living;
                return;
            }
        }

        target = null;
    }

    @EventTarget
    public void onRenderWorld(RenderWorldEvent event) {
        LivingEntity entity = target;
        if (entity == null || mc.player == null || !entity.isAlive()) return;

        float partialTick = event.getPartialTick();
        float ticks = (float) (System.currentTimeMillis() % 1000000L) * 0.004f;
        float pulse = 0.35f + 0.65f * ((Mth.sin(ticks * 1.8f) + 1.0f) * 0.5f) * alphaFactor.get().floatValue();
        float ringRadius = radius.get().floatValue();
        float skirtBottom = (float) (-Math.sin(ticks + 1.0f) / 2.7f);

        Vec3 camera = mc.gameRenderer.getMainCamera().getPosition();
        double x = Mth.lerp(partialTick, entity.xo, entity.getX()) - camera.x;
        double y = Mth.lerp(partialTick, entity.yo, entity.getY()) - camera.y + Math.sin(ticks) + 1.0;
        double z = Mth.lerp(partialTick, entity.zo, entity.getZ()) - camera.z;

        boolean depthTest = !through.get() && mc.player.hasLineOfSight(entity);

        PoseStack poseStack = event.getPoseStack();
        poseStack.pushPose();
        poseStack.translate(x, y, z);
        Matrix4f matrix = poseStack.last().pose();

        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.disableCull();
        RenderSystem.depthMask(false);
        if (depthTest) {
            RenderSystem.enableDepthTest();
        } else {
            RenderSystem.disableDepthTest();
        }
        RenderSystem.setShader(GameRenderer::getPositionColorShader);

        Tesselator tesselator = Tesselator.getInstance();
        BufferBuilder buffer = tesselator.getBuilder();

        buffer.begin(VertexFormat.Mode.TRIANGLE_STRIP, DefaultVertexFormat.POSITION_COLOR);
        for (int i = 0; i <= SEGMENTS; i++) {
            float angle = (float) (Math.PI * 2.0 * i / SEGMENTS);
            float vx = (float) (ringRadius * Math.cos(angle));
            float vz = (float) (ringRadius * Math.sin(angle));
            Color segment = segmentColor(i);
            float r = segment.getRed() / 255.0f;
            float g = segment.getGreen() / 255.0f;
            float b = segment.getBlue() / 255.0f;
            float a = (segment.getAlpha() / 255.0f) * 0.52f * pulse;
            buffer.vertex(matrix, vx, skirtBottom, vz).color(r, g, b, 0.0f).endVertex();
            buffer.vertex(matrix, vx, 0.0f, vz).color(r, g, b, a).endVertex();
        }
        tesselator.end();

        RenderSystem.lineWidth(2.0f);
        buffer.begin(VertexFormat.Mode.DEBUG_LINE_STRIP, DefaultVertexFormat.POSITION_COLOR);
        for (int i = 0; i <= SEGMENTS; i++) {
            float angle = (float) (Math.PI * 2.0 * i / SEGMENTS);
            float vx = (float) (ringRadius * Math.cos(angle));
            float vz = (float) (ringRadius * Math.sin(angle));
            Color segment = segmentColor(i);
            buffer.vertex(matrix, vx, 0.0f, vz)
                    .color(segment.getRed() / 255.0f, segment.getGreen() / 255.0f, segment.getBlue() / 255.0f,
                            (segment.getAlpha() / 255.0f) * pulse)
                    .endVertex();
        }
        tesselator.end();
        RenderSystem.lineWidth(1.0f);

        poseStack.popPose();

        RenderSystem.enableDepthTest();
        RenderSystem.depthMask(true);
        RenderSystem.enableCull();
        RenderSystem.disableBlend();
    }

    private Color segmentColor(int index) {
        return syncColor.get() ? Theme.accent(-index) : color.get();
    }
}
