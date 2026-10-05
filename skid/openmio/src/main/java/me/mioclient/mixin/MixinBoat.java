package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.MixinLivingEntityHelper_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.TickPreEvent;
import me.mioclient.module.movement.EntityControl;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({BoatEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBoat.class */
public abstract class MixinBoat extends Entity implements SearchHelper_4 {
    private static final EntityControl entitycontrol = (EntityControl) BaritoneHelper_3.baritoneHelper_4.getModule117(EntityControl.class);

    @Unique
    private TickPreEvent mio$event;

    public MixinBoat(EntityType<?> entityType, World world) {
        super(entityType, world);
    }

    @Inject(method = {"clampPassengerYaw"}, at = {@At("HEAD")}, cancellable = true)
    public void clampPassengerYawHook(Entity entity, CallbackInfo callbackInfo) {
        if (entitycontrol.is1860(this)) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"updatePaddles"}, at = {@At("HEAD")}, cancellable = true)
    private void updatePaddles(CallbackInfo callbackInfo) {
        if (entitycontrol.is1860(this)) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"tick"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/vehicle/BoatEntity;move(Lnet/minecraft/entity/MovementType;Lnet/minecraft/util/math/Vec3d;)V", shift = At.Shift.BEFORE)})
    private void tickPre(CallbackInfo callbackInfo) {
        if (TickPreEvent.is1301(this)) {
            this.mio$event = new TickPreEvent(getVelocity());
            baritoneHelper.getObject1794(this.mio$event);
            MixinLivingEntityHelper_2.do2581(getVelocity(), this.mio$event.get515(), this.mio$event.get692(), this.mio$event.get516());
        }
    }
}
