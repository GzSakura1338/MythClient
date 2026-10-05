package today.starain.mixin;

import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.gui.screens.multiplayer.JoinMultiplayerScreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import today.starain.ui.injection.InjectedJoinScreen;

@Mixin(Screen.class)
public abstract class MixinScreenKeyForJoin {

    @Inject(method = "keyPressed", at = @At("HEAD"), cancellable = true)
    private void starain$eatEscForInjectedJoin(int keyCode, int scanCode, int modifiers,
                                               CallbackInfoReturnable<Boolean> cir) {
        if (!(((Object) this) instanceof JoinMultiplayerScreen)) return;
        if (!InjectedJoinScreen.isOpen()) return;
        if (keyCode != 256) return;
        InjectedJoinScreen.close();
        cir.setReturnValue(true);
    }
}
