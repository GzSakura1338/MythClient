package today.starain.mixin;

import net.minecraft.client.Minecraft;
import net.minecraft.client.MouseHandler;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.event.impl.MouseButtonEvent;

@Mixin(MouseHandler.class)
public abstract class MixinMouseHandler {
    @Shadow
    @Final
    private Minecraft minecraft;

    @Inject(method = "onPress", at = @At("HEAD"))
    private void starain$onPress(long window, int button, int action, int modifiers, CallbackInfo ci) {
        if (Starain.getInstance() == null || Starain.getInstance().getEventManager() == null) {
            return;
        }

        if (window != minecraft.getWindow().getWindow()) {
            return;
        }

        Starain.getInstance().getEventManager().call(
                new MouseButtonEvent(button, action, modifiers, minecraft.screen));
    }
}
