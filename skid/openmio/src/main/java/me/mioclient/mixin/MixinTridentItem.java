package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.exploit.Trident;
import net.minecraft.item.TridentItem;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Constant;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.ModifyConstant;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({TridentItem.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinTridentItem.class */
public class MixinTridentItem {
    private static final Trident trident = (Trident) BaritoneHelper_3.baritoneHelper_4.getModule117(Trident.class);

    @ModifyExpressionValue(method = {"onStoppedUsing"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/player/PlayerEntity;isTouchingWaterOrRain()Z")})
    private boolean onStoppedUsingHook(boolean z) {
        return z || trident.is2239();
    }

    @ModifyExpressionValue(method = {"use"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/player/PlayerEntity;isTouchingWaterOrRain()Z")})
    private boolean useHook(boolean z) {
        return z || trident.is2239();
    }

    @ModifyArgs(method = {"onStoppedUsing"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/entity/player/PlayerEntity;addVelocity(DDD)V"))
    private void onStoppedUsingHook(Args args) {
        args.set(0, Double.valueOf(((Double) args.get(0)).doubleValue() * trident.get2238()));
        args.set(1, Double.valueOf(((Double) args.get(1)).doubleValue() * trident.get2238()));
        args.set(2, Double.valueOf(((Double) args.get(2)).doubleValue() * trident.get2238()));
    }

    @ModifyConstant(method = {"onStoppedUsing"}, constant = {@Constant(intValue = 10)})
    private int onStoppedUsing(int i) {
        if (trident.isToggled()) {
            return 0;
        }
        return i;
    }
}
