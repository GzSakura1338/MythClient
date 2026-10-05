package dev.myth.mixin;

import dev.myth.Myth;
import dev.myth.event.impl.KeyboardEvent;
import net.minecraft.client.KeyboardHandler;
import net.minecraft.client.Minecraft;
import net.minecraft.client.input.KeyEvent;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(KeyboardHandler.class)
public abstract class MixinKeyboardHandler {
    @Shadow @Final private Minecraft minecraft;

    @Inject(method = "keyPress", at = @At("HEAD"), cancellable = true)
    private void myth$keyPress(long window, int action, KeyEvent key, CallbackInfo ci) {
        if (window == minecraft.getWindow().handle()) {
            Myth.EVENT_MANAGER.call(new KeyboardEvent(key.key(), key.scancode(), action,
                    key.modifiers(), minecraft.gui.screen()));
            if (Myth.UI_MANAGER.key(window, action, key)) ci.cancel();
        }
    }
}
