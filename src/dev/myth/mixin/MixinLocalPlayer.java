package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.wrapmethod.WrapMethod;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import dev.myth.Myth;
import dev.myth.model.combat.KillAura;
import dev.myth.event.api.EventType;
import dev.myth.event.impl.MotionEvent;
import dev.myth.event.impl.PlayerTickEvent;
import dev.myth.event.impl.RespawnEvent;
import dev.myth.event.impl.SlowdownEvent;
import dev.myth.access.MovementInputAccess;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(LocalPlayer.class)
public abstract class MixinLocalPlayer {
    @Unique private MotionEvent myth$motion;

    @Inject(method = "crit", at = @At("HEAD"))
    private void myth$criticalSound(Entity target, CallbackInfo ci) {
        if ((Object) this == net.minecraft.client.Minecraft.getInstance().player)
            Myth.MODEL_MANAGER.getModel(KillAura.class).ifPresent(KillAura::onCriticalHit);
    }

    @Inject(method = "tick", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/AbstractClientPlayer;tick()V"), cancellable = true)
    private void myth$playerTick(CallbackInfo ci) {
        if (Myth.EVENT_MANAGER.call(new PlayerTickEvent()).isCancelled()) {
            ci.cancel();
            return;
        }
        if (((LocalPlayer) (Object) this).tickCount == 0) {
            Myth.EVENT_MANAGER.call(new RespawnEvent());
        }
    }

    @WrapMethod(method = "sendPosition")
    private void myth$motion(Operation<Void> original) {
        LocalPlayer player = (LocalPlayer) (Object) this;
        MotionEvent event = Myth.EVENT_MANAGER.call(new MotionEvent(EventType.PRE,
                player.getX(), player.getY(), player.getZ(), player.getYRot(), player.getXRot(), player.onGround()));
        MotionEvent previous = myth$motion;
        myth$motion = event;
        try {
            if (!event.isCancelled()) {
                original.call();
            }
        } finally {
            myth$motion = previous;
            Myth.EVENT_MANAGER.call(new MotionEvent(EventType.POST, event.getYaw(), event.getPitch()));
        }
    }

    @ModifyExpressionValue(method = "sendPosition", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;getX()D"))
    private double myth$x(double original) {
        return myth$motion == null ? original : myth$motion.getX();
    }

    @ModifyExpressionValue(method = "sendPosition", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;getY()D"))
    private double myth$y(double original) {
        return myth$motion == null ? original : myth$motion.getY();
    }

    @ModifyExpressionValue(method = "sendPosition", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;getZ()D"))
    private double myth$z(double original) {
        return myth$motion == null ? original : myth$motion.getZ();
    }

    @ModifyExpressionValue(method = "sendPosition", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;position()Lnet/minecraft/world/phys/Vec3;"))
    private Vec3 myth$position(Vec3 original) {
        return myth$motion == null ? original : new Vec3(myth$motion.getX(), myth$motion.getY(), myth$motion.getZ());
    }

    @ModifyExpressionValue(method = "sendPosition", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;getYRot()F"))
    private float myth$yaw(float original) {
        return myth$motion == null ? original : myth$motion.getYaw();
    }

    @ModifyExpressionValue(method = "sendPosition", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;getXRot()F"))
    private float myth$pitch(float original) {
        return myth$motion == null ? original : myth$motion.getPitch();
    }

    @ModifyExpressionValue(method = "sendPosition", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;onGround()Z"))
    private boolean myth$onGround(boolean original) {
        return myth$motion == null ? original : myth$motion.isOnGround();
    }

    @ModifyExpressionValue(method = {"modifyInput", "isSlowDueToUsingItem"}, at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;isUsingItem()Z"))
    private boolean myth$slowdown(boolean original) {
        return original && !Myth.EVENT_MANAGER.call(new SlowdownEvent()).isCancelled();
    }

    @ModifyExpressionValue(method = "modifyInput", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;getAttributeValue(Lnet/minecraft/core/Holder;)D"))
    private double myth$sneakMultiplier(double original) {
        return ((LocalPlayer) (Object) this).input instanceof MovementInputAccess access
                ? access.myth$getSneakMultiplier() : original;
    }
}
