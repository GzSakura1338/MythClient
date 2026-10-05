package today.starain.mixin;

import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.level.Level;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;
import today.starain.Starain;
import today.starain.event.impl.AttackSlowdownEvent;
import today.starain.event.impl.AttackYawEvent;

@Mixin({Player.class})
public abstract class MixinPlayer extends LivingEntity {
    protected MixinPlayer(EntityType<? extends LivingEntity> pEntityType, Level pLevel) {
        super(pEntityType, pLevel);
    }

    @Redirect(
            method = {"attack"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/player/Player;getYRot()F"
            )
    )
    private float hookFixRotation(Player instance) {
        AttackYawEvent event = new AttackYawEvent(instance.getYRot());
        Starain.getInstance().getEventManager().call(event);
        return event.getYaw();
    }

    @Redirect(
            method = {"attack"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/player/Player;setDeltaMovement(Lnet/minecraft/world/phys/Vec3;)V"
            )
    )
    private void hookSetDeltaMovement(Player instance, Vec3 vec3) {
        AttackSlowdownEvent event = new AttackSlowdownEvent();
        Starain.getInstance().getEventManager().call(event);
        if (!event.isCancelled()) {
            instance.setDeltaMovement(vec3);
        }
    }

    @Redirect(
            method = {"attack"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/player/Player;setSprinting(Z)V"
            )
    )
    private void hookSetSprinting(Player instance, boolean sprinting) {
        AttackSlowdownEvent event = new AttackSlowdownEvent();
        Starain.getInstance().getEventManager().call(event);
        if (!event.isCancelled()) {
            instance.setSprinting(sprinting);
        }
    }

//    @Inject(
//            method = {"isStayingOnGroundSurface"},
//            at = {@At("RETURN")},
//            cancellable = true
//    )
//    private void isStayingOnGroundSurface(CallbackInfoReturnable<Boolean> info) {
//        EventStayingOnGroundSurface event = new EventStayingOnGroundSurface((Boolean)info.getReturnValue());
//        Naven.getInstance().getEventManager().call(event);
//        info.setReturnValue(event.isStay());
//    }
}
