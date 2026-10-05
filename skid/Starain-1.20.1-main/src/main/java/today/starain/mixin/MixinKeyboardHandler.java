package today.starain.mixin;

import net.minecraft.client.KeyboardHandler;
import net.minecraft.client.Minecraft;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.event.impl.KeyboardEvent;

@Mixin(KeyboardHandler.class)
public abstract class MixinKeyboardHandler {
    @Shadow
    @Final
    private Minecraft minecraft;

    @Inject(method = "keyPress", at = @At("HEAD"))
    private void starain$onKeyPress(long window, int key, int scancode, int action, int modifiers, CallbackInfo ci) {
        if (Starain.getInstance() == null || Starain.getInstance().getEventManager() == null) {
            return;
        }

        // 输入法候选窗等其他窗口也会回调到这里，只认主窗口。
        if (window != minecraft.getWindow().getWindow()) {
            return;
        }

        Starain.getInstance().getEventManager().call(
                new KeyboardEvent(key, scancode, action, modifiers, minecraft.screen));
    }
}
