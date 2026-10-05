package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.SearchHelper4_8;
import me.mioclient.SearchHelper_2;
import me.mioclient.module.PlayerModel;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.ingame.InventoryScreen;
import net.minecraft.entity.LivingEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({InventoryScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinInventoryScreen.class */
public class MixinInventoryScreen {
    private static PlayerModel playermodel = (PlayerModel) BaritoneHelper_3.baritoneHelper_4.getModule117(PlayerModel.class);

    @Inject(method = {"drawEntity(Lnet/minecraft/client/gui/DrawContext;IIIIIFFFLnet/minecraft/entity/LivingEntity;)V"}, at = {@At("HEAD")})
    private static void drawEntityHook(DrawContext drawContext, int i, int i2, int i3, int i4, int i5, float f, float f2, float f3, LivingEntity livingEntity, CallbackInfo callbackInfo) {
        SearchHelper4_8.flag = true;
    }

    @Inject(method = {"drawEntity(Lnet/minecraft/client/gui/DrawContext;IIIIIFFFLnet/minecraft/entity/LivingEntity;)V"}, at = {@At("RETURN")})
    private static void drawEntityHook2(DrawContext drawContext, int i, int i2, int i3, int i4, int i5, float f, float f2, float f3, LivingEntity livingEntity, CallbackInfo callbackInfo) {
        SearchHelper4_8.flag = false;
    }

    @ModifyArgs(method = {"method_29977"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/render/entity/EntityRenderDispatcher;render(Lnet/minecraft/entity/Entity;DDDFFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"))
    private static void lambdaHook(Args args) {
        if (PlayerModel.flag) {
            args.set(5, Float.valueOf(SearchHelper_2.get536()));
        }
    }
}
