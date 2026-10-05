package today.starain.utils.render.shader.impl;

import com.mojang.blaze3d.platform.GlStateManager;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.BufferBuilder;
import com.mojang.blaze3d.vertex.BufferUploader;
import com.mojang.blaze3d.vertex.DefaultVertexFormat;
import com.mojang.blaze3d.vertex.Tesselator;
import com.mojang.blaze3d.vertex.VertexFormat;
import net.minecraft.client.renderer.GameRenderer;
import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.blaze3d.vertex.VertexSorting;
import com.mojang.math.Axis;
import net.minecraft.client.renderer.RenderType;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.LightTexture;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.client.renderer.blockentity.BlockEntityRenderer;
import net.minecraft.client.renderer.entity.EntityRenderDispatcher;
import net.minecraft.client.renderer.texture.OverlayTexture;
import net.minecraft.util.Mth;
import net.minecraft.core.BlockPos;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.world.phys.Vec3;
import org.joml.Matrix4f;
import today.starain.utils.render.shader.Framebuffer;
import today.starain.utils.render.shader.PostProcessRenderer;
import today.starain.utils.render.shader.Shader;
import today.starain.utils.render.shader.ShaderHelper;

import java.lang.reflect.Field;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import static org.lwjgl.opengl.GL11C.GL_COLOR_BUFFER_BIT;
import static org.lwjgl.opengl.GL11C.GL_ONE;
import static org.lwjgl.opengl.GL11C.GL_ONE_MINUS_SRC_ALPHA;
import static org.lwjgl.opengl.GL11C.GL_SRC_ALPHA;

public class OutlineGlow {

    public static final OutlineGlow INSTANCE = new OutlineGlow();

    private final MultiBufferSource.BufferSource maskBuffer = createMaskBuffer();

    private static MultiBufferSource.BufferSource createMaskBuffer() {
        Map<RenderType, BufferBuilder> fixed = new LinkedHashMap<>();
        put(fixed, RenderType.glint());
        put(fixed, RenderType.glintDirect());
        put(fixed, RenderType.glintTranslucent());
        put(fixed, RenderType.entityGlint());
        put(fixed, RenderType.entityGlintDirect());
        put(fixed, RenderType.armorGlint());
        put(fixed, RenderType.armorEntityGlint());
        return MultiBufferSource.immediateWithBuffers(fixed, new BufferBuilder(2048));
    }

    private static void put(Map<RenderType, BufferBuilder> map, RenderType renderType) {
        map.put(renderType, new BufferBuilder(renderType.bufferSize()));
    }

    private static void bobHurt(PoseStack poseStack, float partialTick) {
        Minecraft mc = Minecraft.getInstance();
        if (!(mc.getCameraEntity() instanceof LivingEntity living)) {
            return;
        }

        float hurt = (float) living.hurtTime - partialTick;
        if (living.isDeadOrDying()) {
            float death = Math.min((float) living.deathTime + partialTick, 20.0F);
            poseStack.mulPose(Axis.ZP.rotationDegrees(40.0F - 8000.0F / (death + 200.0F)));
        }
        if (hurt < 0.0F) {
            return;
        }

        hurt /= (float) living.hurtDuration;
        hurt = Mth.sin(hurt * hurt * hurt * hurt * (float) Math.PI);
        float dir = living.getHurtDir();
        poseStack.mulPose(Axis.YP.rotationDegrees(-dir));
        float tilt = (float) ((double) (-hurt) * 14.0 * mc.options.damageTiltStrength().get());
        poseStack.mulPose(Axis.ZP.rotationDegrees(tilt));
        poseStack.mulPose(Axis.YP.rotationDegrees(dir));
    }

    private static void bobView(PoseStack poseStack, Player player, float partialTick) {
        float walked = player.walkDist - player.walkDistO;
        float phase = -(player.walkDist + walked * partialTick);
        float amount = Mth.lerp(partialTick, player.oBob, player.bob);
        poseStack.translate(Mth.sin(phase * (float) Math.PI) * amount * 0.5F,
                -Math.abs(Mth.cos(phase * (float) Math.PI) * amount), 0.0F);
        poseStack.mulPose(Axis.ZP.rotationDegrees(Mth.sin(phase * (float) Math.PI) * amount * 3.0F));
        poseStack.mulPose(Axis.XP.rotationDegrees(Math.abs(Mth.cos(phase * (float) Math.PI - 0.2F) * amount) * 5.0F));
    }

    private static Object irisHandRenderer;
    private static Field irisSolidField;
    private static boolean irisChecked;

    private Shader shader;
    private Shader maskShader;
    private Framebuffer fbo;
    private int width;
    private int height;

    @SuppressWarnings("unchecked")
    public void draw(PoseStack poseStack, float partialTick, List<? extends Entity> targets,
                     List<? extends BlockEntity> blockTargets,
                     float[] colors, int stops, float alpha, float radius, float strength, boolean antiAliasing,
                     boolean renderHand, boolean additive, boolean innerGlow) {
        Minecraft mc = Minecraft.getInstance();
        if (mc.level == null || (targets.isEmpty() && blockTargets.isEmpty() && !renderHand)) {
            return;
        }

        if (shader == null) {
            shader = new Shader("glow.vert", "glow.frag");
        }

        prepareFbo(mc, antiAliasing);

        fbo.bind();
        fbo.setViewport();
        GlStateManager._clearColor(0.0F, 0.0F, 0.0F, 0.0F);
        GlStateManager._clear(GL_COLOR_BUFFER_BIT, Minecraft.ON_OSX);

        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.disableDepthTest();
        RenderSystem.depthMask(false);

        EntityRenderDispatcher dispatcher = mc.getEntityRenderDispatcher();
        dispatcher.setRenderShadow(false);

        Vec3 camera = mc.gameRenderer.getMainCamera().getPosition();
        for (Entity player : targets) {
            try {
                double dx = Mth.lerp(partialTick, player.xo, player.getX()) - camera.x;
                double dy = Mth.lerp(partialTick, player.yo, player.getY()) - camera.y;
                double dz = Mth.lerp(partialTick, player.zo, player.getZ()) - camera.z;
                float yaw = Mth.lerp(partialTick, player.yRotO, player.getYRot());
                dispatcher.render(player, dx, dy, dz, yaw, partialTick, poseStack, maskBuffer, LightTexture.FULL_BRIGHT);
            } catch (Throwable ignored) {
            }
        }

        for (BlockEntity blockEntity : blockTargets) {
            try {
                BlockPos pos = blockEntity.getBlockPos();
                poseStack.pushPose();
                poseStack.translate(pos.getX() - camera.x, pos.getY() - camera.y, pos.getZ() - camera.z);
                BlockEntityRenderer<BlockEntity> blockRenderer =
                        (BlockEntityRenderer<BlockEntity>) mc.getBlockEntityRenderDispatcher().getRenderer(blockEntity);
                if (blockRenderer != null) {
                    blockRenderer.render(blockEntity, partialTick, poseStack, maskBuffer,
                            LightTexture.FULL_BRIGHT, OverlayTexture.NO_OVERLAY);
                }
                poseStack.popPose();
            } catch (Throwable ignored) {
            }
        }

        fbo.bind();
        maskBuffer.endBatch();
        dispatcher.setRenderShadow(true);

        if (renderHand && mc.player != null) {
            Matrix4f oldProjection = RenderSystem.getProjectionMatrix();
            VertexSorting oldSorting = RenderSystem.getVertexSorting();
            double fov = mc.gameRenderer.getFov(mc.gameRenderer.getMainCamera(), partialTick, false);
            RenderSystem.setProjectionMatrix(mc.gameRenderer.getProjectionMatrix(fov), VertexSorting.DISTANCE_TO_ORIGIN);

            poseStack.pushPose();
            poseStack.setIdentity();
            bobHurt(poseStack, partialTick);
            if (mc.options.bobView().get()) {
                bobView(poseStack, mc.player, partialTick);
            }
            Boolean irisSolid = beginIrisHand();
            try {
                mc.gameRenderer.itemInHandRenderer.renderHandsWithItems(partialTick, poseStack, maskBuffer,
                        mc.player, LightTexture.FULL_BRIGHT);
            } catch (Throwable ignored) {
            } finally {
                endIrisHand(irisSolid);
            }
            poseStack.popPose();

            fbo.bind();
            maskBuffer.endBatch();

            RenderSystem.setProjectionMatrix(oldProjection, oldSorting);
        }

        applyGlowPass(mc, colors, stops, alpha, radius, strength, additive, innerGlow, false);
    }

    public void drawRects(List<float[]> rects, float[] colors, int stops, float alpha,
                          float radius, float strength, boolean antiAliasing, boolean additive, boolean innerGlow,
                          boolean vertical) {
        Minecraft mc = Minecraft.getInstance();
        if (rects.isEmpty()) {
            return;
        }
        if (shader == null) {
            shader = new Shader("glow.vert", "glow.frag");
        }
        if (maskShader == null) {
            maskShader = new Shader("glow.vert", "glowmask.frag");
        }

        prepareFbo(mc, antiAliasing);

        fbo.bind();
        fbo.setViewport();
        GlStateManager._clearColor(0.0F, 0.0F, 0.0F, 0.0F);
        GlStateManager._clear(GL_COLOR_BUFFER_BIT, Minecraft.ON_OSX);

        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.disableDepthTest();
        RenderSystem.depthMask(false);

        int count = Math.min(rects.size(), 64);
        float[] data = new float[count * 6];
        for (int i = 0; i < count; i++) {
            float[] r = rects.get(i);
            data[i * 6] = r[0];
            data[i * 6 + 1] = r[1];
            data[i * 6 + 2] = r[2];
            data[i * 6 + 3] = r[3];
            data[i * 6 + 4] = r.length > 4 ? r[4] : 0.0F;
            data[i * 6 + 5] = r.length > 5 ? r[5] : 1.0F;
        }

        maskShader.bind();
        maskShader.set("uRects", data);
        maskShader.set("uRectCount", count);
        maskShader.set("uScreen", mc.getWindow().getGuiScaledWidth(), mc.getWindow().getGuiScaledHeight());
        PostProcessRenderer.beginRender();
        PostProcessRenderer.render();
        PostProcessRenderer.endRender();
        BufferUploader.reset();
        ShaderHelper.useProgram(0);

        applyGlowPass(mc, colors, stops, alpha, radius, strength, additive, innerGlow, vertical);
    }

    private static void initIris() {
        if (irisChecked) {
            return;
        }
        irisChecked = true;
        String[] candidates = {
                "net.irisshaders.iris.pathways.HandRenderer",
                "net.coderbot.iris.pipeline.HandRenderer"
        };
        for (String name : candidates) {
            try {
                Class<?> cls = Class.forName(name);
                Object instance = cls.getField("INSTANCE").get(null);
                Field field = cls.getDeclaredField("renderingSolid");
                field.setAccessible(true);
                irisHandRenderer = instance;
                irisSolidField = field;
                return;
            } catch (Throwable ignored) {
            }
        }
    }

    private static Boolean beginIrisHand() {
        initIris();
        if (irisHandRenderer == null || irisSolidField == null) {
            return null;
        }
        try {
            boolean previous = irisSolidField.getBoolean(irisHandRenderer);
            irisSolidField.setBoolean(irisHandRenderer, true);
            return previous;
        } catch (Throwable ignored) {
            return null;
        }
    }

    private static void endIrisHand(Boolean previous) {
        if (previous == null || irisHandRenderer == null || irisSolidField == null) {
            return;
        }
        try {
            irisSolidField.setBoolean(irisHandRenderer, previous);
        } catch (Throwable ignored) {
        }
    }

    private void prepareFbo(Minecraft mc, boolean antiAliasing) {
        double multi = antiAliasing ? 2.0 : 1.0;
        int windowWidth = mc.getWindow().getWidth();
        int windowHeight = mc.getWindow().getHeight();
        if (fbo == null) {
            fbo = new Framebuffer(multi, true);
        } else if (fbo.sizeMulti != multi || windowWidth != width || windowHeight != height) {
            fbo.sizeMulti = multi;
            fbo.resize();
        }
        width = windowWidth;
        height = windowHeight;
    }

    private void applyGlowPass(Minecraft mc, float[] colors, int stops, float alpha,
                               float radius, float strength, boolean additive, boolean innerGlow, boolean vertical) {
        mc.getMainRenderTarget().bindWrite(true);

        shader.bind();
        ShaderHelper.bindTexture(fbo.texture);
        shader.set("uMask", 0);
        shader.set("uTexelSize", 1.0 / fbo.width, 1.0 / fbo.height);
        shader.set("uColors", colors);
        shader.set("uCount", stops);
        shader.set("uAlpha", alpha);
        shader.set("uRadius", radius * fbo.sizeMulti);
        shader.set("uStrength", strength);
        shader.set("uInner", innerGlow ? 1.0 : 0.0);
        shader.set("uVertical", vertical ? 1.0 : 0.0);

        RenderSystem.enableBlend();
        GlStateManager._blendFunc(GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
        RenderSystem.disableDepthTest();
        RenderSystem.depthMask(false);

        PostProcessRenderer.beginRender();
        PostProcessRenderer.render();
        PostProcessRenderer.endRender();

        BufferUploader.reset();
        ShaderHelper.useProgram(0);
        ShaderHelper.resetTextureSlot();

        RenderSystem.defaultBlendFunc();
        RenderSystem.disableBlend();
        RenderSystem.enableDepthTest();
        RenderSystem.depthMask(true);
        RenderSystem.enableCull();
        RenderSystem.setShaderColor(1.0F, 1.0F, 1.0F, 1.0F);
    }
}
