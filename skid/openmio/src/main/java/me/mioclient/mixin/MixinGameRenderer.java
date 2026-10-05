package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.v2.WrapWithCondition;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import com.llamalad7.mixinextras.sugar.Local;
import com.mojang.blaze3d.systems.RenderSystem;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ESPSearchHelper4_3;
import me.mioclient.FreecamHelper;
import me.mioclient.MixinLivingEntityHelper_2;
import me.mioclient.NewChunksHelper_4;
import me.mioclient.SearchHelper_4;
import me.mioclient.ShaderSearchHelper4;
import me.mioclient.module.exploit.Reach;
import me.mioclient.module.player.Freecam;
import me.mioclient.module.render.FreeLook;
import me.mioclient.module.render.NoBob;
import me.mioclient.module.render.NoRender;
import me.mioclient.module.render.Shader;
import me.mioclient.module.render.ViewModel;
import me.mioclient.module.render.Zoom;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.client.render.BufferBuilderStorage;
import net.minecraft.client.render.Camera;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.RenderTickCounter;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.item.HeldItemRenderer;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.MathHelper;
import org.joml.Matrix4f;
import org.joml.Quaternionf;
import org.joml.Vector3f;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({GameRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinGameRenderer.class */
public abstract class MixinGameRenderer {
    private static Zoom zoom = (Zoom) BaritoneHelper_3.baritoneHelper_4.getModule117(Zoom.class);
    private static Reach reach = (Reach) BaritoneHelper_3.baritoneHelper_4.getModule117(Reach.class);
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static Shader shader = (Shader) BaritoneHelper_3.baritoneHelper_4.getModule117(Shader.class);
    private static Freecam freecam = (Freecam) BaritoneHelper_3.baritoneHelper_4.getModule117(Freecam.class);
    private static FreeLook freelook = (FreeLook) BaritoneHelper_3.baritoneHelper_4.getModule117(FreeLook.class);
    private static ViewModel viewmodel = (ViewModel) BaritoneHelper_3.baritoneHelper_4.getModule117(ViewModel.class);
    private static NoBob nobob = (NoBob) BaritoneHelper_3.baritoneHelper_4.getModule117(NoBob.class);

    @Shadow
    @Final
    MinecraftClient field_4015;

    @Shadow
    @Final
    private Camera field_18765;

    @Shadow
    private boolean field_3992;

    @Shadow
    @Final
    public HeldItemRenderer field_4012;

    @Shadow
    @Final
    private BufferBuilderStorage field_20948;

    @Unique
    private boolean bobbing;

    @Unique
    private boolean prevBobbing;
    private boolean freecamSet = false;

    @Shadow
    private void method_3172(Camera camera, float f, Matrix4f matrix4f) {
    }

    @Shadow
    public abstract void method_3182();

    @Shadow
    public abstract void method_3190(float f);

    @Inject(at = {@At(value = "FIELD", target = "Lnet/minecraft/client/render/GameRenderer;renderHand:Z", opcode = 180, ordinal = 0)}, method = {"renderWorld"})
    private void renderWorldFieldHook(RenderTickCounter renderTickCounter, CallbackInfo callbackInfo) {
        if (this.bobbing) {
            this.field_4015.options.getBobView().setValue(true);
            this.bobbing = false;
        }
    }

    @Inject(at = {@At("HEAD")}, method = {"renderWorld"})
    private void renderWorldPreHook(RenderTickCounter renderTickCounter, CallbackInfo callbackInfo) {
        this.prevBobbing = true;
    }

    @Inject(at = {@At("HEAD")}, method = {"bobView"}, cancellable = true)
    private void bobViewHook(MatrixStack matrixStack, float f, CallbackInfo callbackInfo) {
        if (((Boolean)(Object) this.field_4015.options.getBobView().getValue()).booleanValue() && this.prevBobbing) {
            this.bobbing = true;
            this.field_4015.options.getBobView().setValue(false);
            this.prevBobbing = false;
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"tiltViewWhenHurt"}, at = {@At("HEAD")}, cancellable = true)
    private void bobViewWhenHurtHook(MatrixStack matrixStack, float f, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.hurtCam.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"showFloatingItem"}, at = {@At("HEAD")}, cancellable = true)
    private void showFloatingItemHook(ItemStack itemStack, CallbackInfo callbackInfo) {
        if (itemStack.getItem() == Items.TOTEM_OF_UNDYING && norender.isToggled() && norender.totemOverlay.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @WrapOperation(method = {"renderWorld"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/util/math/MathHelper;lerp(FFF)F")}, require = 0)
    private float get1953(float f, float f2, float f3, Operation<Float> operation) {
        if (norender.isToggled() && norender.blindness.getValue().booleanValue()) {
            return 0.0f;
        }
        return MathHelper.lerp(f, f2, f3);
    }

    @Inject(method = {"findCrosshairTarget"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/projectile/ProjectileUtil;raycast(Lnet/minecraft/entity/Entity;Lnet/minecraft/util/math/Vec3d;Lnet/minecraft/util/math/Vec3d;Lnet/minecraft/util/math/Box;Ljava/util/function/Predicate;D)Lnet/minecraft/util/hit/EntityHitResult;")}, cancellable = true)
    private void onUpdateTargetedEntity(Entity entity, double d, double d2, float f, CallbackInfoReturnable<HitResult> callbackInfoReturnable, @Local HitResult hitResult) {
        if (reach.is951() && hitResult.getType() == HitResult.Type.BLOCK) {
            callbackInfoReturnable.setReturnValue(hitResult);
        }
    }

    @WrapWithCondition(method = {"renderHand"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/item/HeldItemRenderer;renderItem(FLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider$Immediate;Lnet/minecraft/client/network/ClientPlayerEntity;I)V")})
    private boolean renderHand(HeldItemRenderer heldItemRenderer, float f, MatrixStack matrixStack, VertexConsumerProvider.Immediate immediate, ClientPlayerEntity clientPlayerEntity, int i) {
        if (!ShaderSearchHelper4.flag2) {
            return true;
        }
        ESPSearchHelper4_3.do2887(shader.shader.getValue().getShaderFramebufferHelper21(), true, () -> {
            heldItemRenderer.renderItem(f, matrixStack, immediate, clientPlayerEntity, i);
        });
        return false;
    }

    @Inject(method = {"renderWorld"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/GameRenderer;renderHand(Lnet/minecraft/client/render/Camera;FLorg/joml/Matrix4f;)V", shift = At.Shift.BEFORE)}, cancellable = true)
    private void renderWorldHook(RenderTickCounter renderTickCounter, CallbackInfo callbackInfo, @Local(ordinal = 1) Matrix4f matrix4f, @Local(ordinal = 0) float f, @Local(ordinal = 0) Quaternionf quaternionf) {
        if (this.field_3992 && shader.isToggled() && shader.hands.getValue().booleanValue()) {
            callbackInfo.cancel();
            if (!norender.is179()) {
                method_3172(this.field_18765, f, matrix4f);
            }
            ShaderSearchHelper4.flag2 = true;
            method_3172(this.field_18765, f, matrix4f);
            ShaderSearchHelper4.flag2 = false;
            SearchHelper_4.minecraftClient.getProfiler().pop();
        }
    }

    @Redirect(method = {"bobView"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;translate(FFF)V"))
    private void mio$redirectTranslate(MatrixStack matrixStack, float f, float f2, float f3) {
        float floatValue = nobob.multiplier.getValue().floatValue();
        if (floatValue <= 0.0f || !nobob.isToggled()) {
            floatValue = 1.0f;
        }
        if (((Boolean)(Object) this.field_4015.options.getBobView().getValue()).booleanValue()) {
            matrixStack.translate(f * floatValue, f2 * floatValue, f3 * floatValue);
        }
    }

    @Inject(method = {"renderHand"}, at = {@At("HEAD")}, cancellable = true)
    private void renderHandHook(Camera camera, float f, Matrix4f matrix4f, CallbackInfo callbackInfo) {
        RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, norender.get1995());
        if (viewmodel.isToggled() && !viewmodel.shadow.getValue().booleanValue()) {
            float pitch = camera.getPitch() * FreecamHelper.val4;
            float f2 = (-(camera.getYaw() - 45.0f)) * FreecamHelper.val4;
            float cos = MathHelper.cos(f2);
            float sin = MathHelper.sin(f2);
            float cos2 = MathHelper.cos(pitch);
            Vector3f vector3f = new Vector3f(sin * cos2, -MathHelper.sin(pitch), cos * cos2);
            RenderSystem.setShaderLights(vector3f, new Vector3f(vector3f).mul(-1.0f));
        }
        if (freecam.is179() || norender.is179()) {
            callbackInfo.cancel();
            RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    @WrapOperation(method = {"renderHand"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/GameRenderer;getFov(Lnet/minecraft/client/render/Camera;FZ)D")}, require = 0)
    private double renderHandHook(GameRenderer gameRenderer, Camera camera, float f, boolean z, Operation<Double> operation) {
        double base = ((Double) operation.call(new Object[]{gameRenderer, camera, Float.valueOf(f), Boolean.valueOf(z)})).doubleValue();
        return (viewmodel.isToggled() && viewmodel.viewModelFov.getValue().booleanValue()) ? viewmodel.fovAmount.getValue().intValue() * base : base * (zoom.isToggled() ? 1.0f - zoom.get918() : 1.0f);
    }

    @Inject(method = {"renderHand"}, at = {@At("RETURN")})
    private void renderHandHook2(Camera camera, float f, Matrix4f matrix4f, CallbackInfo callbackInfo) {
        RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    @Inject(method = {"updateCrosshairTarget"}, at = {@At("HEAD")}, cancellable = true)
    private void updateTargetedEntityInvoke(float f, CallbackInfo callbackInfo) {
        if (freecam.isToggled() && this.field_4015.getCameraEntity() != null && !this.freecamSet) {
            callbackInfo.cancel();
            Entity cameraEntity = this.field_4015.getCameraEntity();
            double x = cameraEntity.getX();
            double y = cameraEntity.getY();
            double z = cameraEntity.getZ();
            double d = cameraEntity.prevX;
            double d2 = cameraEntity.prevY;
            double d3 = cameraEntity.prevZ;
            float yaw = cameraEntity.getYaw();
            float pitch = cameraEntity.getPitch();
            float f2 = cameraEntity.prevYaw;
            float f3 = cameraEntity.prevPitch;
            cameraEntity.prevX = freecam.vec3d2.x;
            cameraEntity.prevY = freecam.vec3d2.y - cameraEntity.getEyeHeight(cameraEntity.getPose());
            cameraEntity.prevZ = freecam.vec3d2.z;
            cameraEntity.setYaw(freecam.val);
            cameraEntity.setHeadYaw(freecam.val);
            cameraEntity.setPitch(freecam.val3);
            cameraEntity.prevYaw = freecam.val2;
            cameraEntity.prevPitch = freecam.val4;
            this.freecamSet = true;
            NewChunksHelper_4.do2149(() -> {
                MixinLivingEntityHelper_2.do2581(cameraEntity.getPos(), freecam.vec3d.x, freecam.vec3d.y - cameraEntity.getEyeHeight(cameraEntity.getPose()), freecam.vec3d.z);
                method_3190(f);
                MixinLivingEntityHelper_2.do2581(cameraEntity.getPos(), x, y, z);
            });
            this.freecamSet = false;
            cameraEntity.prevX = d;
            cameraEntity.prevY = d2;
            cameraEntity.prevZ = d3;
            cameraEntity.setYaw(yaw);
            cameraEntity.setPitch(pitch);
            cameraEntity.prevYaw = f2;
            cameraEntity.prevPitch = f3;
        }
        if (!freelook.isToggled() || freecam.isToggled()) {
            return;
        }
        callbackInfo.cancel();
    }

    @Inject(method = {"render"}, at = {@At("HEAD")})
    private void render(RenderTickCounter renderTickCounter, boolean z, CallbackInfo callbackInfo) {
        BaritoneHelper_3.hitmarkerSearchHelper4.do3093();
    }
}
