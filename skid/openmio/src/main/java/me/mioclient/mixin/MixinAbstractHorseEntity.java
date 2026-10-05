package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.movement.EntityControl;
import net.minecraft.entity.passive.AbstractHorseEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({AbstractHorseEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinAbstractHorseEntity.class */
public class MixinAbstractHorseEntity {
    private static EntityControl entitycontrol = (EntityControl) BaritoneHelper_3.baritoneHelper_4.getModule117(EntityControl.class);

    @Inject(method = {"isSaddled"}, at = {@At("HEAD")}, cancellable = true)
    private void isSaddledHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (entitycontrol.isToggled()) {
            callbackInfoReturnable.setReturnValue(true);
        }
    }
}
