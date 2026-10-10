package dev.myth.mixin;

import com.llamalad7.mixinextras.sugar.Local;
import com.mojang.blaze3d.opengl.GlTexture;
import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.textures.GpuTextureView;
import dev.myth.Myth;
import dev.myth.model.render.NoRender;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.client.renderer.SubmitNodeStorage;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import org.joml.Matrix4f;
import org.joml.Matrix4fc;
import org.joml.Vector4f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

import com.mojang.blaze3d.vertex.PoseStack;

/** NoRender's HurtCam, Hands and TotemOverlay, plus the Shader ESP's first person hand pass. */
@Mixin(GameRenderer.class)
public abstract class MixinGameRenderer {
    @Inject(method = "bobHurt", at = @At("HEAD"), cancellable = true)
    private void myth$hurtCam(CameraRenderState cameraState, PoseStack poseStack, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.hurtCam.get()) ci.cancel();
    }

    /** Mio is179: only a zero Opacity skips the whole hand pass. */
    @Inject(method = "renderItemInHand", at = @At("HEAD"), cancellable = true)
    private void myth$hands(CameraRenderState cameraState, float deltaPartialTick, Matrix4fc modelViewMatrix, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.hidesHands()) ci.cancel();
    }

    /**
     * The pose the hand is submitted from, copied before vanilla submits.
     *
     * <p>submitHandsWithItems starts with two mulPose calls for the view sway that it never pops -
     * vanilla gets away with it because renderItemInHand pops the whole stack afterwards. Handing
     * that same stack to a second submit applies the sway twice, and the silhouette then drifts off
     * the hand whenever the view swings, which is invisible while standing still because the term
     * is (viewRot - bob) and goes to zero. So the second pass starts from a copy taken beforehand
     * and lets submitHandsWithItems apply the sway once, exactly as it did for vanilla.
     */
    @Unique private final Matrix4f myth$handPose = new Matrix4f();
    /** Reused the way vanilla reuses handAndScreenSubmitNodeStorage; prepareFrame drains it. */
    @Unique private final SubmitNodeStorage myth$handStorage = new SubmitNodeStorage();
    @Unique private boolean myth$handPoseReady;

    @Inject(method = "renderItemInHand", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/renderer/ItemInHandRenderer;submitHandsWithItems(FLcom/mojang/blaze3d/vertex/PoseStack;Lnet/minecraft/client/renderer/SubmitNodeCollector;Lnet/minecraft/client/player/LocalPlayer;I)V"))
    private void myth$captureHandPose(CameraRenderState cameraState, float deltaPartialTick, Matrix4fc modelViewMatrix,
                                      CallbackInfo ci, @Local PoseStack poseStack) {
        myth$handPose.set(poseStack.last().pose());
        myth$handPoseReady = true;
    }

    /**
     * Shader ESP for the first person hand. The hand never reaches the entity outline target:
     * GameRenderer submits it into its own handAndScreenSubmitNodeStorage and calls
     * renderAllFeatures directly, outside the level's frame graph, and it happens after the world
     * pass has already composited. So the hand is submitted a second time with the outline target
     * as the output - Mio renders it twice for the same reason - and composited on its own.
     *
     * <p>Injected right after vanilla's renderAllFeatures, where the hand's projection and the
     * model view stack are still set up; renderItemInHand pops both before it returns.
     */
    @Inject(method = "renderItemInHand", at = @At(value = "INVOKE", shift = At.Shift.AFTER,
            target = "Lnet/minecraft/client/renderer/feature/FeatureRenderDispatcher;renderAllFeatures(Lnet/minecraft/client/renderer/SubmitNodeStorage;)V"))
    private void myth$handShader(CameraRenderState cameraState, float deltaPartialTick, Matrix4fc modelViewMatrix,
                                 CallbackInfo ci) {
        if (!myth$handPoseReady) return;
        myth$handPoseReady = false;
        dev.myth.model.render.Shader shader = dev.myth.model.render.Shader.active();
        if (shader == null || !shader.hands.get()) return;
        Minecraft client = Minecraft.getInstance();
        if (client.player == null) return;
        RenderTarget outline = ((dev.myth.mixin.access.LevelRendererAccess) client.levelRenderer).myth$entityOutlineTarget();
        RenderTarget main = client.gameRenderer.mainRenderTarget();
        if (outline == null || main == null) return;
        if (!(outline.getColorTexture() instanceof GlTexture source)) return;
        if (!(main.getColorTexture() instanceof GlTexture target)) return;

        // The world pass already consumed the target, so only the hand may be left in it.
        RenderSystem.getDevice().createCommandEncoder().clearColorAndDepthTextures(
                outline.getColorTexture(), new Vector4f(0, 0, 0, 0), outline.getDepthTexture(), 0);
        GpuTextureView color = RenderSystem.outputColorTextureOverride, depth = RenderSystem.outputDepthTextureOverride;
        RenderSystem.outputColorTextureOverride = outline.getColorTextureView();
        RenderSystem.outputDepthTextureOverride = outline.getDepthTextureView();
        try {
            // The silhouette keeps the item's own colours; only its alpha is read, and the shaders
            // replace the colour outright, so there is no need to flatten it to white first.
            PoseStack pose = new PoseStack();
            pose.mulPose(myth$handPose);
            client.gameRenderer.itemInHandRenderer.submitHandsWithItems(deltaPartialTick, pose, myth$handStorage,
                    client.player, client.getEntityRenderDispatcher().getPackedLightCoords(client.player, deltaPartialTick));
            client.gameRenderer.featureRenderDispatcher().renderAllFeatures(myth$handStorage);
        } finally {
            RenderSystem.outputColorTextureOverride = color;
            RenderSystem.outputDepthTextureOverride = depth;
        }
        Myth.ENTITY_SHADER.composite(source.glId(), target.glId(), main.width, main.height, shader.style(true));
    }

    @Inject(method = "displayItemActivation", at = @At("HEAD"), cancellable = true)
    private void myth$totemOverlay(ItemStack itemStack, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.totemOverlay.get() && itemStack.is(Items.TOTEM_OF_UNDYING)) ci.cancel();
    }
}
