package dev.myth.mixin;

import dev.myth.Myth;
import dev.myth.event.impl.MouseButtonEvent;
import net.minecraft.client.Minecraft;
import net.minecraft.client.MouseHandler;
import net.minecraft.client.input.MouseButtonInfo;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(MouseHandler.class)
public abstract class MixinMouseHandler {
    @Shadow @Final private Minecraft minecraft;

    @Inject(method = "onButton", at = @At("HEAD"), cancellable = true)
    private void myth$mouseButton(long window, MouseButtonInfo button, int action, CallbackInfo ci) {
        if (window == minecraft.getWindow().handle()) {
            Myth.EVENT_MANAGER.call(new MouseButtonEvent(button.button(), action,
                    button.modifiers(), minecraft.gui.screen()));
            if (Myth.UI_MANAGER.button(window, button.button(), action, button.modifiers())) ci.cancel();
        }
    }
}
