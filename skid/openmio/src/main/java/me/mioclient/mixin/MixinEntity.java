package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.ModifyReceiver;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.CameraYawHelper;
import me.mioclient.FreecamHelper;
import me.mioclient.KeyPearlMode;
import me.mioclient.SearchHelper_3;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.MoveEvent_2;
import me.mioclient.module.movement.ElytraFly;
import me.mioclient.module.movement.NoSlow;
import me.mioclient.module.movement.Velocity;
import me.mioclient.module.player.Freecam;
import me.mioclient.module.render.FreeLook;
import me.mioclient.module.render.NoRender;
import net.minecraft.block.Block;
import net.minecraft.block.Blocks;
import net.minecraft.client.MinecraftClient;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityPose;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.MovementType;
import net.minecraft.fluid.FluidState;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({Entity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinEntity.class */
public abstract class MixinEntity implements CameraYawHelper {

    @Shadow
    private float field_6031;

    @Shadow
    private float field_5965;

    @Unique
    private float cameraPitch;

    @Unique
    private float cameraYaw;

    @Unique
    private Vec3d mio$prevVelocity;
    private static NoSlow noslow = (NoSlow) BaritoneHelper_3.baritoneHelper_4.getModule117(NoSlow.class);
    private static ElytraFly elytrafly = (ElytraFly) BaritoneHelper_3.baritoneHelper_4.getModule117(ElytraFly.class);
    private static Freecam freecam = (Freecam) BaritoneHelper_3.baritoneHelper_4.getModule117(Freecam.class);
    private static Velocity velo = (Velocity) BaritoneHelper_3.baritoneHelper_4.getModule117(Velocity.class);
    private static FreeLook fl = (FreeLook) BaritoneHelper_3.baritoneHelper_4.getModule117(FreeLook.class);
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @Shadow
    private Vec3d method_17835(Vec3d vec3d) {
        return null;
    }

    @Shadow
    public abstract void method_5784(MovementType movementType, Vec3d vec3d);

    @Shadow
    public abstract boolean equals(Object obj);

    @Shadow
    public abstract void method_33574(Vec3d vec3d);

    @Shadow
    public abstract void method_5814(double d, double d2, double d3);

    @Shadow
    protected abstract void method_5710(float f, float f2);

    @Shadow
    public abstract Vec3d method_18798();

    @Shadow
    public abstract boolean method_41328(EntityPose entityPose);

    @ModifyReceiver(method = {"getVelocityMultiplier"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/block/Block;getVelocityMultiplier()F")}, require = 0)
    private Block getVelocityMultiplierHook(Block block) {
        return ((Object) this) != MinecraftClient.getInstance().player ? block : (noslow.isToggled() && noslow.soulSand.getValue().booleanValue()) ? Blocks.STONE : block;
    }

    @Inject(method = {"pushAwayFrom"}, at = {@At("HEAD")}, cancellable = true)
    private void pushAwayFromHook(Entity entity, CallbackInfo callbackInfo) {
        if (equals(MinecraftClient.getInstance().player) && velo.isToggled() && velo.push.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @Redirect(method = {"updateMovementInFluid"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/fluid/FluidState;getVelocity(Lnet/minecraft/world/BlockView;Lnet/minecraft/util/math/BlockPos;)Lnet/minecraft/util/math/Vec3d;"))
    private Vec3d updateMovementInFluidHook(FluidState fluidState, BlockView blockView, BlockPos blockPos) {
        return (velo.isToggled() && velo.liquids.getValue().booleanValue()) ? Vec3d.ZERO : fluidState.getVelocity(blockView, blockPos);
    }

    @Redirect(method = {"move"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/entity/Entity;adjustMovementForCollisions(Lnet/minecraft/util/math/Vec3d;)Lnet/minecraft/util/math/Vec3d;"))
    private Vec3d moveHook(Entity entity, Vec3d vec3d) {
        if (((Object) this) != MinecraftClient.getInstance().player) {
            return method_17835(vec3d);
        }
        MoveEvent_2 moveEvent_2 = new MoveEvent_2(KeyPearlMode.Pre, 0.6f);
        SearchHelper_4.baritoneHelper.getObject1794(moveEvent_2);
        SearchHelper_3.do649((LivingEntity)(Object) this, moveEvent_2.get990());
        Vec3d method_17835 = method_17835(vec3d);
        if (method_17835 != null) {
            SearchHelper_4.baritoneHelper.getObject1794(new MoveEvent_2(KeyPearlMode.Post, (float) method_17835.y));
        }
        return method_17835;
    }

    @Inject(method = {"getPose"}, at = {@At("HEAD")}, cancellable = true)
    private void getPoseHook(CallbackInfoReturnable<EntityPose> callbackInfoReturnable) {
        if (((Object) this) != MinecraftClient.getInstance().player) {
            return;
        }
        if (elytrafly.is949() || elytrafly.is956()) {
            callbackInfoReturnable.setReturnValue(EntityPose.STANDING);
            callbackInfoReturnable.cancel();
        }
    }

    @Inject(method = {"changeLookDirection"}, at = {@At("HEAD")}, cancellable = true)
    public void changeCameraLookDirection(double d, double d2, CallbackInfo callbackInfo) {
        if (!freecam.isToggled() && fl.isToggled() && equals(MinecraftClient.getInstance().player)) {
            this.cameraPitch = MathHelper.clamp(this.cameraPitch + ((float) (d2 * 0.15d)), -FreecamHelper.num2, FreecamHelper.num2);
            this.cameraYaw += (float) (d * 0.15d);
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"lerpPosAndRotation"}, at = {@At("HEAD")}, cancellable = true)
    private void lerpPosAndRotationHook(int i, double d, double d2, double d3, double d4, double d5, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.interpolation.getValue().booleanValue()) {
            method_5814(d, d2, d3);
            method_5710((float) d4, (float) d5);
            callbackInfo.cancel();
        }
    }

    @ModifyExpressionValue(method = {"getCameraPosVec"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/Entity;getStandingEyeHeight()F")})
    private float getHeightHook(float f) {
        if (norender.isToggled() && norender.newSneaking.getValue().booleanValue() && method_41328(EntityPose.CROUCHING)) {
            return 1.54f;
        }
        return f;
    }

    @Inject(method = {"baseTick"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/util/profiler/Profiler;push(Ljava/lang/String;)V", ordinal = 0, shift = At.Shift.AFTER)})
    private void baseTick(CallbackInfo callbackInfo) {
        this.mio$prevVelocity = method_18798();
    }

    @Inject(method = {"<init>"}, at = {@At("TAIL")})
    private void init(EntityType entityType, World world, CallbackInfo callbackInfo) {
        this.mio$prevVelocity = Vec3d.ZERO;
    }

    @Override // me.mioclient.CameraYawHelper
    @Unique
    public float getCameraPitch() {
        return this.cameraPitch;
    }

    @Override // me.mioclient.CameraYawHelper
    @Unique
    public float getCameraYaw() {
        return this.cameraYaw;
    }

    @Override // me.mioclient.CameraYawHelper
    @Unique
    public void setCameraPitch(float f) {
        this.cameraPitch = f;
    }

    @Override // me.mioclient.CameraYawHelper
    @Unique
    public void setCameraYaw(float f) {
        this.cameraYaw = f;
    }

    @Override // me.mioclient.CameraYawHelper
    public Vec3d mio$getPrevVelocity() {
        return this.mio$prevVelocity;
    }
}
