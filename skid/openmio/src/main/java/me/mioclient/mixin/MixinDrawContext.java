package me.mioclient.mixin;

import com.llamalad7.mixinextras.sugar.Local;
import java.util.List;
import java.util.Optional;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.MixinDrawContextHelper;
import me.mioclient.TooltipsData;
import me.mioclient.module.render.Tooltips;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.tooltip.TooltipComponent;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.item.ItemStack;
import net.minecraft.item.tooltip.TooltipData;
import net.minecraft.text.Text;
import org.jetbrains.annotations.Nullable;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({DrawContext.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinDrawContext.class */
public abstract class MixinDrawContext {

    @Unique
    private static Tooltips tooltips = (Tooltips) BaritoneHelper_3.baritoneHelper_4.getModule117(Tooltips.class);

    @Unique
    private boolean translate = false;

    @Shadow
    public abstract void method_51427(ItemStack itemStack, int i, int i2);

    @Shadow
    public abstract MatrixStack method_51448();

    @Inject(at = {@At(value = "INVOKE", target = "net/minecraft/item/ItemStack.isItemBarVisible()Z")}, method = {"drawItemInSlot(Lnet/minecraft/client/font/TextRenderer;Lnet/minecraft/item/ItemStack;IILjava/lang/String;)V"})
    private void drawItemInSlotHook(TextRenderer textRenderer, ItemStack itemStack, int i, int i2, @Nullable String str, CallbackInfo callbackInfo) {
        if (tooltips.isToggled() && tooltips.majorityItem.getValue().booleanValue()) {
            for (TooltipsData tooltipsData : tooltips.list) {
                if (tooltipsData.getItemStack2242() == itemStack && tooltipsData.getItemStack2243() != null) {
                    this.translate = true;
                    method_51427(tooltipsData.getItemStack2243(), i, i2);
                    this.translate = false;
                }
            }
        }
    }

    @ModifyArgs(method = {"drawItem(Lnet/minecraft/entity/LivingEntity;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;IIII)V"}, at = @At(value = "INVOKE", target = "net/minecraft/client/util/math/MatrixStack.translate(FFF)V"))
    private void drawItemHook(Args args) {
        if (this.translate) {
            args.set(0, Float.valueOf(((Float) args.get(0)).floatValue() + 3.5f));
            args.set(1, Float.valueOf(((Float) args.get(1)).floatValue() - 4.0f));
            args.set(2, Float.valueOf(((Float) args.get(2)).floatValue() + 100.0f));
        }
    }

    @ModifyArgs(method = {"drawItem(Lnet/minecraft/entity/LivingEntity;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;IIII)V"}, at = @At(value = "INVOKE", target = "net/minecraft/client/util/math/MatrixStack.scale(FFF)V"))
    private void drawItemHook2(Args args) {
        if (this.translate) {
            args.set(0, Float.valueOf(10.0f));
            args.set(1, Float.valueOf(-10.0f));
        }
    }

    @Inject(method = {"drawTooltip(Lnet/minecraft/client/font/TextRenderer;Ljava/util/List;Ljava/util/Optional;II)V"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/DrawContext;drawTooltip(Lnet/minecraft/client/font/TextRenderer;Ljava/util/List;IILnet/minecraft/client/gui/tooltip/TooltipPositioner;)V", shift = At.Shift.BEFORE)}, cancellable = true)
    private void drawTooltipHook(TextRenderer textRenderer, List<Text> list, Optional<TooltipData> optional, int i, int i2, CallbackInfo callbackInfo, @Local(ordinal = 1) List<TooltipComponent> list2) {
        if (MixinDrawContextHelper.is1129()) {
            MixinDrawContextHelper.do249(list2);
            callbackInfo.cancel();
        }
    }
}
