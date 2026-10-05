package me.mioclient.mixin;

import me.mioclient.ArrowsHelper;
import net.minecraft.entity.effect.StatusEffect;
import net.minecraft.entity.effect.StatusEffectInstance;
import net.minecraft.registry.entry.RegistryEntry;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({StatusEffectInstance.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinStatusEffectInstance.class */
public class MixinStatusEffectInstance implements ArrowsHelper {

    @Shadow
    private int field_5895;

    @Unique
    private int mio$initialDuration;

    @Inject(method = {"<init>(Lnet/minecraft/registry/entry/RegistryEntry;IIZZZLnet/minecraft/entity/effect/StatusEffectInstance;)V"}, at = {@At("TAIL")})
    private void initHook(RegistryEntry<StatusEffect> registryEntry, int i, int i2, boolean z, boolean z2, boolean z3, StatusEffectInstance statusEffectInstance, CallbackInfo callbackInfo) {
        this.mio$initialDuration = i;
    }

    @Inject(method = {"copyFrom"}, at = {@At("TAIL")})
    private void copyFrom(StatusEffectInstance statusEffectInstance, CallbackInfo callbackInfo) {
        this.mio$initialDuration = statusEffectInstance.getDuration();
    }

    @Inject(method = {"upgrade"}, at = {@At(value = "FIELD", target = "Lnet/minecraft/entity/effect/StatusEffectInstance;duration:I", opcode = 181, shift = At.Shift.AFTER)})
    private void updateDuration(CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        this.mio$initialDuration = this.field_5895;
    }

    @Override // me.mioclient.ArrowsHelper
    public float mio$getDurationRation() {
        return this.field_5895 / this.mio$initialDuration;
    }
}
