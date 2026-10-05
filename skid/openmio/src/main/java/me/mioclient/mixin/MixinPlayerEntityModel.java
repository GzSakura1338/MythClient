package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.render.entity.model.PlayerEntityModel;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

/* compiled from: 0.java */
@Mixin({PlayerEntityModel.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinPlayerEntityModel.class */
public class MixinPlayerEntityModel {
    private static final NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @ModifyExpressionValue(method = {"setAngles(Lnet/minecraft/entity/LivingEntity;FFFFF)V"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/item/ItemStack;isEmpty()Z")})
    private boolean setAnglesHook(boolean z) {
        if (norender.get1996() == 0.0f) {
            return true;
        }
        return z;
    }
}
