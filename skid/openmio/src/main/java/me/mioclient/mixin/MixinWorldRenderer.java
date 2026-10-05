package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.sugar.Local;
import com.mojang.blaze3d.systems.RenderSystem;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FramebufferHelper_2;
import me.mioclient.FramebufferHelper_4;
import me.mioclient.MatrixStackEvent;
import me.mioclient.SearchHelper_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.ShaderSearchHelper4;
import me.mioclient.event.DrawBlockOutlineEvent;
import me.mioclient.module.player.Freecam;
import me.mioclient.module.render.Ambience;
import me.mioclient.module.render.Chams;
import me.mioclient.module.render.NoRender;
import me.mioclient.module.render.SkyColor;
import me.mioclient.module.render.ViewClip;
import net.minecraft.block.BlockState;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gl.Framebuffer;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.client.render.BufferBuilderStorage;
import net.minecraft.client.render.Camera;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.LightmapTextureManager;
import net.minecraft.client.render.RenderTickCounter;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.WorldRenderer;
import net.minecraft.client.render.block.entity.BlockEntityRenderDispatcher;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.client.world.ClientWorld;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.BlockPos;
import org.jetbrains.annotations.Nullable;
import org.joml.Matrix4f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyVariable;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({WorldRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinWorldRenderer.class */
public abstract class MixinWorldRenderer implements SearchHelper_4, FramebufferHelper_4 {
    private static final ViewClip viewclip = (ViewClip) BaritoneHelper_3.baritoneHelper_4.getModule117(ViewClip.class);
    private static Ambience ambience = (Ambience) BaritoneHelper_3.baritoneHelper_4.getModule117(Ambience.class);
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static Freecam freecam = (Freecam) BaritoneHelper_3.baritoneHelper_4.getModule117(Freecam.class);
    private static SkyColor skycolor = (SkyColor) BaritoneHelper_3.baritoneHelper_4.getModule117(SkyColor.class);
    private static Chams chams = (Chams) BaritoneHelper_3.baritoneHelper_4.getModule117(Chams.class);

    @Unique
    private MatrixStack mio$stack;

    @Shadow
    private Framebuffer field_4101;

    @Shadow
    @Nullable
    private ClientWorld field_4085;

    @Shadow
    protected abstract void method_22977(Entity entity, double d, double d2, double d3, float f, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider);

    @Shadow
    private void method_22712(MatrixStack matrixStack, VertexConsumer vertexConsumer, Entity entity, double d, double d2, double d3, BlockPos blockPos, BlockState blockState) {
    }

    @Inject(method = {"renderWeather"}, at = {@At("HEAD")}, cancellable = true)
    private void renderWeatherHook(LightmapTextureManager lightmapTextureManager, float f, double d, double d2, double d3, CallbackInfo callbackInfo) {
        if (ambience.isToggled() && ambience.worldWeather.getValue().booleanValue() && ambience.weather.getValue() == Ambience.AmbiencePredicateMode.CLEAR) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"render"}, at = {@At("RETURN")})
    private void render(RenderTickCounter renderTickCounter, boolean z, Camera camera, GameRenderer gameRenderer, LightmapTextureManager lightmapTextureManager, Matrix4f matrix4f, Matrix4f matrix4f2, CallbackInfo callbackInfo, @Local MatrixStack matrixStack) {
        matrixStack.push();
        SearchHelper_2.do578(matrixStack);
        RenderSystem.clear(256, MinecraftClient.IS_SYSTEM_MAC);
        RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
        MatrixStackEvent.Inner_3 inner_3133 = MatrixStackEvent.Inner_3.getInner_3133(matrixStack, SearchHelper_2.get536());
        SearchHelper_2.searchHelper_2.do566(() -> {
            baritoneHelper.getObject1794(inner_3133);
        });
        baritoneHelper.getObject1794(MatrixStackEvent.Inner_2.getInner_23016(matrixStack, SearchHelper_2.get536()));
        matrixStack.pop();
    }

    @Inject(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/BufferBuilderStorage;getEntityVertexConsumers()Lnet/minecraft/client/render/VertexConsumerProvider$Immediate;", shift = At.Shift.BEFORE)})
    private void render_pre(RenderTickCounter renderTickCounter, boolean z, Camera camera, GameRenderer gameRenderer, LightmapTextureManager lightmapTextureManager, Matrix4f matrix4f, Matrix4f matrix4f2, CallbackInfo callbackInfo, @Local MatrixStack matrixStack) {
        baritoneHelper.getObject1794(MatrixStackEvent.Inner.getInner933(matrixStack, SearchHelper_2.get536()));
    }

    @Redirect(method = {"render"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/render/WorldRenderer;drawBlockOutline(Lnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumer;Lnet/minecraft/entity/Entity;DDDLnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/BlockState;)V"))
    private void render_drawBlockOutline(WorldRenderer worldRenderer, MatrixStack matrixStack, VertexConsumer vertexConsumer, Entity entity, double d, double d2, double d3, BlockPos blockPos, BlockState blockState) {
        DrawBlockOutlineEvent drawBlockOutlineEvent = new DrawBlockOutlineEvent(matrixStack, vertexConsumer, blockPos, blockState);
        baritoneHelper.getObject1794(drawBlockOutlineEvent);
        if (drawBlockOutlineEvent.is2403()) {
            return;
        }
        method_22712(drawBlockOutlineEvent.getMatrixStack1486(), drawBlockOutlineEvent.getVertexConsumer1488(), entity, d, d2, d3, drawBlockOutlineEvent.getBlockPos386(), drawBlockOutlineEvent.getBlockState670());
    }

    @Inject(method = {"hasBlindnessOrDarkness"}, at = {@At("HEAD")}, cancellable = true)
    private void invokeBlindnessHook(Camera camera, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (norender.isToggled() && norender.blindness.getValue().booleanValue()) {
            callbackInfoReturnable.setReturnValue(null);
        }
    }

    @Redirect(method = {"render"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isSpectator()Z"))
    private boolean renderHook(ClientPlayerEntity clientPlayerEntity) {
        if (freecam.isToggled()) {
            return true;
        }
        if (viewclip.isToggled() && minecraftClient.gameRenderer.getCamera().isThirdPerson()) {
            return true;
        }
        return clientPlayerEntity.isSpectator();
    }

    @Inject(method = {"<init>"}, at = {@At("RETURN")})
    private void initHook(MinecraftClient minecraftClient, EntityRenderDispatcher entityRenderDispatcher, BlockEntityRenderDispatcher blockEntityRenderDispatcher, BufferBuilderStorage bufferBuilderStorage, CallbackInfo callbackInfo) {
        FramebufferHelper_2.init();
        ShaderSearchHelper4.init();
    }

    @Inject(method = {"render"}, at = {@At("HEAD")})
    private void onRenderHead(RenderTickCounter renderTickCounter, boolean z, Camera camera, GameRenderer gameRenderer, LightmapTextureManager lightmapTextureManager, Matrix4f matrix4f, Matrix4f matrix4f2, CallbackInfo callbackInfo) {
        SearchHelper_2.do537(renderTickCounter.getTickDelta(false));
        this.mio$stack = new MatrixStack();
        SearchHelper_2.do538(this.mio$stack);
        ShaderSearchHelper4.do760();
    }

    @Inject(method = {"render"}, at = {@At("TAIL")})
    private void onRenderTail(RenderTickCounter renderTickCounter, boolean z, Camera camera, GameRenderer gameRenderer, LightmapTextureManager lightmapTextureManager, Matrix4f matrix4f, Matrix4f matrix4f2, CallbackInfo callbackInfo) {
        ShaderSearchHelper4.do866();
    }

    @Inject(method = {"renderEntity"}, at = {@At("HEAD")}, cancellable = true)
    private void renderEntity(Entity entity, double d, double d2, double d3, float f, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, CallbackInfo callbackInfo) {
        if ((norender.entities2.getValue().booleanValue() && norender.is1992(entity)) || chams.is2045(entity)) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"onResized"}, at = {@At("HEAD")})
    private void onResized(int i, int i2, CallbackInfo callbackInfo) {
        ShaderSearchHelper4.do762(i, i2);
    }

    @Inject(method = {"renderWorldBorder"}, at = {@At("HEAD")}, cancellable = true)
    private void renderWorldBorderHook(Camera camera, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.worldBorder.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @Override // me.mioclient.FramebufferHelper_4
    public Framebuffer getFramebuffer() {
        return this.field_4101;
    }

    @Override // me.mioclient.FramebufferHelper_4
    public void setFramebuffer(Framebuffer framebuffer) {
        this.field_4101 = framebuffer;
    }

    @ModifyVariable(method = {"getLightmapCoordinates(Lnet/minecraft/world/BlockRenderView;Lnet/minecraft/block/BlockState;Lnet/minecraft/util/math/BlockPos;)I"}, at = @At("STORE"), ordinal = 0)
    private static int getLightmapCoordinates(int i) {
        return (ambience.isToggled() && ambience.brightness.getValue() == Ambience.MixinEntityRendererMode.SKY) ? Math.max(ambience.lightLevel.getValue().intValue(), i) : Math.max(0, i);
    }

    @Redirect(method = {"render"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/render/Camera;isThirdPerson()Z"))
    private boolean renderHook(Camera camera) {
        if (freecam.isToggled()) {
            return true;
        }
        return camera.isThirdPerson();
    }

    @ModifyExpressionValue(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/DimensionEffects;isDarkened()Z")})
    private boolean isDarkened(boolean z) {
        return (skycolor.isToggled() && skycolor.is3136()) ? ((me.mioclient.MixinWorldRendererHelper)(Object) this.field_4085).mio$getOriginalEffects().isDarkened() : z;
    }

    @Inject(method = {"renderWeather"}, at = {@At("HEAD")})
    private void renderWeatherHook2(LightmapTextureManager lightmapTextureManager, float f, double d, double d2, double d3, CallbackInfo callbackInfo) {
        RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 0.25f);
    }

    @Inject(method = {"renderWeather"}, at = {@At("TAIL")})
    private void renderWeatherHook3(LightmapTextureManager lightmapTextureManager, float f, double d, double d2, double d3, CallbackInfo callbackInfo) {
        RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    @ModifyExpressionValue(method = {"renderWeather"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/world/biome/Biome;hasPrecipitation()Z")})
    private boolean renderWeatherHook(boolean z) {
        return z || ambience.is2924();
    }

    @ModifyExpressionValue(method = {"renderWeather"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/world/World;getTopY(Lnet/minecraft/world/Heightmap$Type;II)I")})
    private int renderWeatherHook(int i) {
        if (!ambience.is2924()) {
            return i;
        }
        int i2 = 5;
        if (MinecraftClient.isFancyGraphicsOrBetter()) {
            i2 = 10;
        }
        return (((int) Math.floor(minecraftClient.gameRenderer.getCamera().getPos().y)) - i2) - 1;
    }
}
