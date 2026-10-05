package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.mojang.authlib.GameProfile;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.Helper_17;
import me.mioclient.KeyPearlMode;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.MoveEvent;
import me.mioclient.event.PushOutOfBlocksEvent;
import me.mioclient.event.SprintUpdateEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.event.TickMovementEvent;
import me.mioclient.event.TickPostEvent;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.misc.Swing;
import me.mioclient.module.movement.ElytraFly;
import me.mioclient.module.movement.EntityControl;
import me.mioclient.module.movement.NoSlow;
import me.mioclient.module.movement.Sprint;
import net.minecraft.client.input.Input;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.client.world.ClientWorld;
import net.minecraft.entity.MovementType;
import net.minecraft.network.packet.c2s.play.HandSwingC2SPacket;
import net.minecraft.util.Hand;
import net.minecraft.util.math.Vec3d;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({ClientPlayerEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinClientPlayerEntity.class */
public abstract class MixinClientPlayerEntity extends AbstractClientPlayerEntity implements SearchHelper_4, Helper_17 {
    private static ElytraFly elytrafly = (ElytraFly) BaritoneHelper_3.baritoneHelper_4.getModule117(ElytraFly.class);
    private static Swing swing = (Swing) BaritoneHelper_3.baritoneHelper_4.getModule117(Swing.class);
    private static NoSlow noslow = (NoSlow) BaritoneHelper_3.baritoneHelper_4.getModule117(NoSlow.class);
    private static EntityControl entitycontrol = (EntityControl) BaritoneHelper_3.baritoneHelper_4.getModule117(EntityControl.class);
    private static Sprint sprint = (Sprint) BaritoneHelper_3.baritoneHelper_4.getModule117(Sprint.class);

    @Shadow
    public Input field_3913;

    @Unique
    private MotionEvent motionEvent;

    public MixinClientPlayerEntity(ClientWorld clientWorld, GameProfile gameProfile) {
        super(clientWorld, gameProfile);
        this.motionEvent = new MotionEvent();
    }

    @Shadow
    protected abstract void method_3148(float f, float f2);

    @Shadow
    public abstract float getPitch(float f);

    @Shadow
    public abstract float getYaw(float f);

    @Shadow
    public abstract void method_33689();

    @Shadow
    public abstract boolean isUsingItem();

    @Shadow
    private void method_3136() {
    }

    @Inject(method = {"swingHand"}, at = {@At("HEAD")}, cancellable = true)
    public void swingHandHook(Hand hand, CallbackInfo callbackInfo) {
        if (!swing.isToggled() || swing.hand.getValue() == Swing.ScaffoldHelperMode.VANILLA) {
            return;
        }
        if (swing.hand.getValue() != Swing.ScaffoldHelperMode.PACKET) {
            super.swingHand(swing.hand.getValue().getHand2084());
        }
        minecraftClient.player.networkHandler.sendPacket(new HandSwingC2SPacket(hand));
        callbackInfo.cancel();
    }

    @Inject(method = {"tick"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/AbstractClientPlayerEntity;tick()V", shift = At.Shift.BEFORE)})
    public void tickPost(CallbackInfo callbackInfo) {
        if (minecraftClient.player == null || minecraftClient.world == null) {
            return;
        }
        baritoneHelper.getObject1794(new TickPostEvent());
    }

    @Inject(method = {"tick"}, at = {@At("RETURN")})
    public void tick(CallbackInfo callbackInfo) {
        if (minecraftClient.player == null || minecraftClient.world == null) {
            return;
        }
        baritoneHelper.getObject1794(new TickEvent());
    }

    @Inject(method = {"move"}, at = {@At("HEAD")}, cancellable = true)
    private void move(MovementType movementType, Vec3d vec3d, CallbackInfo callbackInfo) {
        Vec3d vec3d2 = new Vec3d(vec3d.x, vec3d.y, vec3d.z);
        MoveEvent moveEvent = new MoveEvent(vec3d, movementType);
        baritoneHelper.getObject1794(moveEvent);
        if (moveEvent.is2403()) {
            callbackInfo.cancel();
            return;
        }
        if (vec3d2.equals(moveEvent.getVec3d689())) {
            return;
        }
        callbackInfo.cancel();
        double x = getX();
        double z = getZ();
        super.move(moveEvent.getMovementType693(), moveEvent.getVec3d689());
        method_3148((float) (getX() - x), (float) (getZ() - z));
    }

    @Inject(method = {"tick"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/AbstractClientPlayerEntity;tick()V", shift = At.Shift.AFTER)}, cancellable = true)
    private void tickPreHook(CallbackInfo callbackInfo) {
        resetEvent();
        baritoneHelper.getObject1794(this.motionEvent);
        setPosition(this.motionEvent.get515(), this.motionEvent.get692(), this.motionEvent.get516());
        setYaw(this.motionEvent.get2255());
        setPitch(this.motionEvent.get2256());
        setOnGround(this.motionEvent.is2228());
        if (this.motionEvent.is2403()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"tick"}, at = {@At(value = "INVOKE", target = "Ljava/util/List;iterator()Ljava/util/Iterator;", shift = At.Shift.BEFORE)}, cancellable = true)
    private void tickPostHook(CallbackInfo callbackInfo) {
        if (this.motionEvent.get515() != this.motionEvent.get2249() || this.motionEvent.get692() != this.motionEvent.get2250() || this.motionEvent.get516() != this.motionEvent.get2251()) {
            setPosition(new Vec3d(this.motionEvent.get2249(), this.motionEvent.get2250(), this.motionEvent.get2251()));
        }
        resetRotations();
        if (isOnGround() == this.motionEvent.is2228()) {
            setOnGround(this.motionEvent.is2254());
        }
    }

    @Inject(method = {"sendMovementPackets"}, at = {@At("RETURN")})
    private void sendMovementPacketsPostHook(CallbackInfo callbackInfo) {
        MotionEvent motionEvent = new MotionEvent(KeyPearlMode.Post, this.motionEvent);
        if (this.motionEvent.is2403()) {
            motionEvent.do1162();
        }
        baritoneHelper.getObject1794(motionEvent);
    }

    @ModifyExpressionValue(method = {"sendMovementPackets"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;getX()D")})
    private double getXHook(double d) {
        return this.motionEvent.get515();
    }

    @ModifyExpressionValue(method = {"sendMovementPackets"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;getY()D")})
    private double getYHook(double d) {
        return this.motionEvent.get692();
    }

    @ModifyExpressionValue(method = {"sendMovementPackets"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;getZ()D")})
    private double getZHook(double d) {
        return this.motionEvent.get516();
    }

    @ModifyExpressionValue(method = {"sendMovementPackets"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;getYaw()F")})
    private float getYawHook(float f) {
        if (this.motionEvent.get2252() == f && this.motionEvent.get2252() != this.motionEvent.get751()) {
            return this.motionEvent.get751();
        }
        return f;
    }

    @ModifyExpressionValue(method = {"sendMovementPackets"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;getPitch()F")})
    private float getPitchHook(float f) {
        if (this.motionEvent.get2253() == f && this.motionEvent.get2253() != this.motionEvent.get752()) {
            return this.motionEvent.get752();
        }
        return f;
    }

    @ModifyExpressionValue(method = {"sendMovementPackets"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isOnGround()Z")})
    private boolean isOnGroundHook(boolean z) {
        return this.motionEvent.is2228();
    }

    @Redirect(method = {"tickMovement"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;setSprinting(Z)V", ordinal = 3), require = 0)
    private void setSprintingHook(ClientPlayerEntity clientPlayerEntity, boolean z) {
        clientPlayerEntity.setSprinting(sprint.isToggled() && sprint.is2181());
    }

    @Inject(method = {"tickMovement"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;canStartSprinting()Z")})
    private void onSprintUpdate(CallbackInfo callbackInfo) {
        baritoneHelper.getObject1794(new SprintUpdateEvent());
    }

    @ModifyExpressionValue(method = {"tickMovement"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isUsingItem()Z")})
    private boolean tickMovementHook(boolean z) {
        TickMovementEvent tickMovementEvent = new TickMovementEvent();
        baritoneHelper.getObject1794(tickMovementEvent);
        if (tickMovementEvent.is2403()) {
            return false;
        }
        return z;
    }

    @Inject(method = {"pushOutOfBlocks"}, at = {@At("HEAD")}, cancellable = true)
    private void pushOutOfBlocksHook(double d, double d2, CallbackInfo callbackInfo) {
        PushOutOfBlocksEvent pushOutOfBlocksEvent = new PushOutOfBlocksEvent();
        baritoneHelper.getObject1794(pushOutOfBlocksEvent);
        if (pushOutOfBlocksEvent.is2403()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"getMountJumpStrength"}, at = {@At("HEAD")}, cancellable = true)
    private void getJumpMountStrengthHook(CallbackInfoReturnable<Float> callbackInfoReturnable) {
        if (entitycontrol.isToggled() && entitycontrol.horseJump.getValue().booleanValue()) {
            callbackInfoReturnable.setReturnValue(Float.valueOf(1.0f));
        }
    }

    @Inject(method = {"tickNausea"}, at = {@At("HEAD")})
    private void updateNauseaHook(CallbackInfo callbackInfo) {
        if (noslow.isToggled() && noslow.portals.getValue().booleanValue() && this.portalManager != null) {
            this.portalManager.setInPortal(false);
        }
    }

    @Inject(method = {"tickMovement"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isFallFlying()Z", shift = At.Shift.BEFORE)})
    private void tickMovementElytra(CallbackInfo callbackInfo) {
        if (elytrafly.isToggled() && elytrafly.is606()) {
            boolean z = minecraftClient.player.input.jumping;
            if ((elytrafly.mode.getValue() != ElytraFly.ElytraFlyPredicateMode.PACKET || (elytrafly.vertical2.getValue().booleanValue() && minecraftClient.player.input.jumping)) && z) {
                elytrafly.do948();
            } else {
                BaritoneHelper_3.holeSnapSearchHelper4_2.do2017(elytrafly);
            }
        }
    }

    @Inject(method = {"shouldSlowDown"}, at = {@At("HEAD")}, cancellable = true)
    private void shouldSlowdown(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        boolean z = minecraftClient.player.isCrawling() && noslow.crawl.getValue().booleanValue();
        boolean z2 = !z && minecraftClient.player.isSneaking() && noslow.sneak.getValue().booleanValue();
        if (noslow.isToggled()) {
            if (z || z2) {
                callbackInfoReturnable.setReturnValue(false);
            }
        }
    }

    @Override // me.mioclient.Helper_17
    public void sendMovementPacketsWrapper() {
        method_3136();
    }

    @Override // me.mioclient.Helper_17
    public void superTick() {
        super.tick();
    }

    @Override // me.mioclient.Helper_17
    public void resetEvent() {
        this.motionEvent = new MotionEvent(KeyPearlMode.Pre, getX(), getBoundingBox().minY, getZ(), getYaw(), getPitch(), isOnGround());
    }

    @Override // me.mioclient.Helper_17
    public void resetRotations() {
        BaritoneHelper_3.searchHelper4_8.do2494((!hasVehicle() || this.motionEvent.is855()) ? this.motionEvent.get751() : getVehicle().getYaw(), this.motionEvent.get752());
        if (getYaw() == this.motionEvent.get2255()) {
            setYaw(this.motionEvent.get2252());
        }
        if (getPitch() == this.motionEvent.get2256()) {
            setPitch(this.motionEvent.get2253());
        }
    }
}
