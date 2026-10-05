package me.mioclient.mixin;

import me.mioclient.SearchHelper_4;
import me.mioclient.event.FinishUsingEvent;
import me.mioclient.feature.Items;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({ItemStack.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinItemStack.class */
public abstract class MixinItemStack {
    @Shadow
    public abstract Item method_7909();

    @Inject(method = {"finishUsing"}, at = {@At("HEAD")})
    private void finishUsing(World world, LivingEntity livingEntity, CallbackInfoReturnable<ItemStack> callbackInfoReturnable) {
        if (livingEntity instanceof ClientPlayerEntity) {
            SearchHelper_4.baritoneHelper.getObject1794(new FinishUsingEvent((ItemStack)(Object) this));
        }
    }

    @Inject(method = {"hasGlint"}, at = {@At("HEAD")}, cancellable = true)
    private void hasGlintHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (Items.is1124(method_7909())) {
            callbackInfoReturnable.setReturnValue(true);
            callbackInfoReturnable.cancel();
        }
    }
}
