package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.toast.SystemToast;
import net.minecraft.client.toast.Toast;
import net.minecraft.client.toast.TutorialToast;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin(targets = {"net/minecraft/client/toast/ToastManager$Entry"})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinToastManagerEntry.class */
public class MixinToastManagerEntry {
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @Shadow
    @Final
    private Toast field_2241;

    @Inject(method = {"draw"}, at = {@At("HEAD")}, cancellable = true)
    public void draw(int i, DrawContext drawContext, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (norender.isToggled()) {
            if ((this.field_2241.getType() == SystemToast.Type.UNSECURE_SERVER_WARNING && norender.unsecureServer.getValue().booleanValue()) || ((this.field_2241 instanceof TutorialToast) && norender.tutorialToast.getValue().booleanValue())) {
                callbackInfoReturnable.setReturnValue(true);
            }
        }
    }
}
