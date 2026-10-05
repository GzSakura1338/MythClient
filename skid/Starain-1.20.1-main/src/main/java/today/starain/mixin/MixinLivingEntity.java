package today.starain.mixin;

import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.level.Level;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;
import today.starain.Starain;
import today.starain.event.impl.FallFlyingEvent;
import today.starain.event.impl.JumpEvent;
import today.starain.event.impl.RotationAnimationEvent;

@Mixin({LivingEntity.class})
public abstract class MixinLivingEntity extends Entity {
    public MixinLivingEntity(EntityType<?> pEntityType, Level pLevel) {
        super(pEntityType, pLevel);
    }

    @Redirect(
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/LivingEntity;getYRot()F",
                    opcode = 182,
                    ordinal = 0
            ),
            method = {"jumpFromGround"}
    )
    private float modifyJumpYaw(LivingEntity entity) {
        JumpEvent event = new JumpEvent(entity.getYRot());
        Starain.getInstance().getEventManager().call(event);
        return event.getYaw();
    }

    @Redirect(
            method = {"travel"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/LivingEntity;getXRot()F"
            )
    )
    private float hookModifyFallFlyingPitch(LivingEntity instance) {
        FallFlyingEvent event = new FallFlyingEvent(instance.getXRot());
        Starain.getInstance().getEventManager().call(event);
        return event.getPitch();
    }

//    @Inject(
//            method = {"hasEffect"},
//            at = {@At("HEAD")},
//            cancellable = true
//    )
//    private void hasEffect(MobEffect pEffect, CallbackInfoReturnable<Boolean> cir) {
//        LivingEntity thisEntity = (LivingEntity)(Object)this;
//        if (thisEntity == Minecraft.getInstance().player) {
//            FullBright fullBright = (FullBright)Naven.getInstance().getModuleManager().getModule(FullBright.class);
//            if (pEffect == MobEffects.NIGHT_VISION && fullBright.isEnabled()) {
//                cir.setReturnValue(true);
//                cir.cancel();
//            }
//
//            AntiNausea antiNausea = (AntiNausea)Naven.getInstance().getModuleManager().getModule(AntiNausea.class);
//            if (pEffect == MobEffects.CONFUSION && antiNausea.isEnabled()) {
//                cir.setReturnValue(false);
//                cir.cancel();
//            }
//        }
//    }

    @Redirect(
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/LivingEntity;getYRot()F"
            ),
            method = {"tickHeadTurn"}
    )
    private float modifyHeadYaw(LivingEntity entity) {
        if (entity == Minecraft.getInstance().player) {
            RotationAnimationEvent event = new RotationAnimationEvent(entity.getYRot(), 0.0F, 0.0F, 0.0F);
            Starain.getInstance().getEventManager().call(event);
            return event.getYaw();
        } else {
            return entity.getYRot();
        }
    }
}
