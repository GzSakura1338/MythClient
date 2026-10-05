package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.CameraYawHelper;
import me.mioclient.SearchHelper_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.UpdateSetPosEvent;
import me.mioclient.module.movement.ElytraFly;
import me.mioclient.module.player.Freecam;
import me.mioclient.module.render.FreeLook;
import me.mioclient.module.render.NoRender;
import me.mioclient.module.render.ViewClip;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.client.render.Camera;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityPose;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import org.joml.Vector3f;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({Camera.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinCamera.class */
public abstract class MixinCamera {
    private static ViewClip viewclip = (ViewClip) BaritoneHelper_3.baritoneHelper_4.getModule117(ViewClip.class);
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static ElytraFly elytrafly = (ElytraFly) BaritoneHelper_3.baritoneHelper_4.getModule117(ElytraFly.class);
    private static Freecam freecam = (Freecam) BaritoneHelper_3.baritoneHelper_4.getModule117(Freecam.class);
    private static FreeLook fl = (FreeLook) BaritoneHelper_3.baritoneHelper_4.getModule117(FreeLook.class);

    @Shadow
    @Final
    private Vector3f field_18714;

    @Shadow
    @Final
    private Vector3f field_18715;

    @Shadow
    @Final
    private Vector3f field_18716;

    @Shadow
    private Vec3d field_18712;

    @Shadow
    private float field_18721;

    @Shadow
    private float field_18722;

    @Shadow
    private Entity field_18711;

    @Unique
    private boolean hold = true;

    @Shadow
    protected void method_19322(Vec3d vec3d) {
    }

    @Shadow
    protected abstract void method_19325(float f, float f2);

    @Inject(method = {"clipToSpace"}, at = {@At("HEAD")}, cancellable = true)
    private void clipToSpaceHook(float f, CallbackInfoReturnable<Float> callbackInfoReturnable) {
        if (viewclip.isToggled()) {
            callbackInfoReturnable.setReturnValue(Float.valueOf(viewclip.range.getValue().floatValue()));
        }
    }

    @Inject(method = {"updateEyeHeight"}, at = {@At("HEAD")}, cancellable = true)
    private void updateEyeHeightHook(CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.newSneaking.getValue().booleanValue() && this.field_18711 != null) {
            callbackInfo.cancel();
            this.field_18722 = this.field_18721;
            PlayerEntity playerEntity = (this.field_18711) instanceof PlayerEntity ? (PlayerEntity) (this.field_18711) : null;
            if (playerEntity instanceof PlayerEntity) {
                PlayerEntity playerEntity2 = playerEntity;
                if (playerEntity2.isInPose(EntityPose.CROUCHING)) {
                    this.field_18721 = 1.54f;
                } else if (this.field_18721 >= 1.62f || !playerEntity2.isInPose(EntityPose.STANDING)) {
                    this.field_18721 += (this.field_18711.getStandingEyeHeight() - this.field_18721) * 0.5f;
                } else {
                    this.field_18721 = 1.62f - ((float) ((1.62f - this.field_18721) * 0.4d));
                }
            }
        }
    }

    @Inject(method = {"moveBy"}, at = {@At("HEAD")}, cancellable = true)
    private void moveByHook(float f, float f2, float f3, CallbackInfo callbackInfo) {
        if (viewclip.isToggled() && viewclip.smooth.getValue().booleanValue()) {
            float f4 = viewclip.progress.get172();
            method_19322(new Vec3d(this.field_18712.x + (((this.field_18714.x() * f) + (this.field_18715.x() * f2) + (this.field_18716.x() * f3)) * f4), this.field_18712.y + (((this.field_18714.y() * f) + (this.field_18715.y() * f2) + (this.field_18716.y() * f3)) * f4), this.field_18712.z + (((this.field_18714.z() * f) + (this.field_18715.z() * f2) + (this.field_18716.z() * f3)) * f4)));
            callbackInfo.cancel();
        }
    }

    @ModifyArgs(method = {"update"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/render/Camera;setPos(DDD)V"))
    private void onUpdateSetPosArgs(Args args) {
        UpdateSetPosEvent updateSetPosEvent = new UpdateSetPosEvent(((Double) args.get(0)).doubleValue(), ((Double) args.get(1)).doubleValue(), ((Double) args.get(2)).doubleValue());
        SearchHelper_4.baritoneHelper.getObject1794(updateSetPosEvent);
        if (updateSetPosEvent.is2403()) {
            args.set(0, Double.valueOf(updateSetPosEvent.getVec3d1303().x));
            args.set(1, Double.valueOf(updateSetPosEvent.getVec3d1303().y));
            args.set(2, Double.valueOf(updateSetPosEvent.getVec3d1303().z));
        }
    }

    @ModifyArgs(method = {"update"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/render/Camera;setRotation(FF)V"))
    private void onUpdateSetRotationArgs(Args args) {
        if (elytrafly.isToggled() && elytrafly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.BOUNCE && elytrafly.pitchLock.getValue().booleanValue() && !elytrafly.silent.getValue().booleanValue()) {
            args.set(1, elytrafly.pitch2.getValue());
        }
        float f = SearchHelper_2.get536();
        if (freecam.isToggled()) {
            args.set(0, Float.valueOf((float) freecam.get177(f)));
            args.set(1, Float.valueOf((float) freecam.get178(f)));
        }
    }

    @Inject(method = {"update"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/Camera;setRotation(FF)V", ordinal = 0, shift = At.Shift.AFTER)})
    public void lockRotation(BlockView blockView, Entity entity, boolean z, boolean z2, float f, CallbackInfo callbackInfo) {
        if (freecam.isToggled()) {
            return;
        }
        if (fl.isToggled() && (entity instanceof ClientPlayerEntity)) {
            MinecraftClient minecraftClient = MinecraftClient.getInstance();
            CameraYawHelper cameraYawHelper = (CameraYawHelper) entity;
            if (minecraftClient.player != null && this.hold) {
                cameraYawHelper.setCameraYaw(minecraftClient.player.getYaw());
                cameraYawHelper.setCameraPitch(minecraftClient.player.getPitch());
                this.hold = false;
            }
            method_19325(cameraYawHelper.getCameraYaw(), cameraYawHelper.getCameraPitch());
        }
        if (fl.isToggled() || !(entity instanceof ClientPlayerEntity)) {
            return;
        }
        this.hold = true;
    }
}
