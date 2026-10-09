package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.gui.components.toasts.SystemToast;
import net.minecraft.client.gui.components.toasts.Toast;
import net.minecraft.client.gui.components.toasts.ToastManager;
import net.minecraft.client.gui.components.toasts.TutorialToast;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** Mio drops the toast before it is queued, so nothing animates in. */
@Mixin(ToastManager.class)
public abstract class MixinToastManager {
    @Inject(method = "addToast", at = @At("HEAD"), cancellable = true)
    private void myth$toasts(Toast toast, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender == null) return;
        if (toast instanceof TutorialToast && noRender.tutorialToast.get()) { ci.cancel(); return; }
        if (toast instanceof SystemToast system && noRender.unsecureServer.get()
                && system.getToken() == SystemToast.SystemToastId.UNSECURE_SERVER_WARNING) ci.cancel();
    }
}
