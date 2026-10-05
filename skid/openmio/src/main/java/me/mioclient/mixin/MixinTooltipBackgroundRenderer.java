package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.Tooltips;
import net.minecraft.client.gui.tooltip.TooltipBackgroundRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({TooltipBackgroundRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinTooltipBackgroundRenderer.class */
public class MixinTooltipBackgroundRenderer {
    private static Tooltips tooltips = (Tooltips) BaritoneHelper_3.baritoneHelper_4.getModule117(Tooltips.class);

    @ModifyArgs(method = {"render"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/tooltip/TooltipBackgroundRenderer;renderBorder(Lnet/minecraft/client/gui/DrawContext;IIIIIII)V"))
    private static void renderHook(Args args) {
        if (!tooltips.isToggled() || tooltips.getColor586() == null) {
            return;
        }
        args.set(6, Integer.valueOf(tooltips.getColor586().hashCode()));
        args.set(7, Integer.valueOf(tooltips.getColor586().darker().hashCode()));
        tooltips.do587(null);
    }
}
