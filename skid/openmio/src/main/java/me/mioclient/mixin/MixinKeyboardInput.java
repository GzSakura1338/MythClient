package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.TickEvent_2;
import me.mioclient.event.TickHookPreEvent;
import me.mioclient.mixin.ducks.DuckKeyBinding;
import me.mioclient.module.movement.NoSlow;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.input.Input;
import net.minecraft.client.input.KeyboardInput;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.KeyBinding;
import org.lwjgl.glfw.GLFW;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({KeyboardInput.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinKeyboardInput.class */
public class MixinKeyboardInput extends Input {
    private static NoSlow noslow = (NoSlow) BaritoneHelper_3.baritoneHelper_4.getModule117(NoSlow.class);

    @Shadow
    @Final
    private GameOptions field_3902;

    @Inject(method = {"tick"}, at = {@At("HEAD")}, cancellable = true)
    private void tickHookPre(boolean z, float f, CallbackInfo callbackInfo) {
        TickHookPreEvent tickHookPreEvent = new TickHookPreEvent((KeyboardInput)(Object) this, z ? f : -1.0f);
        SearchHelper_4.baritoneHelper.getObject1794(tickHookPreEvent);
        if (tickHookPreEvent.is2403()) {
            SearchHelper_4.baritoneHelper.getObject1794(new TickEvent_2(tickHookPreEvent.getInput806(), tickHookPreEvent.get807()));
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"tick"}, at = {@At("TAIL")}, cancellable = true)
    private void tickHook(boolean z, float f, CallbackInfo callbackInfo) {
        SearchHelper_4.baritoneHelper.getObject1794(new TickEvent_2((KeyboardInput)(Object) this, z ? f : -1.0f));
    }

    @Redirect(method = {"tick"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/option/KeyBinding;isPressed()Z"), require = 0)
    private boolean tickHook2(KeyBinding keyBinding) {
        return (noslow.isToggled() && noslow.is2669() && noslow.guiMove.getValue().booleanValue() && keyBinding != this.field_3902.sneakKey) ? GLFW.glfwGetKey(MinecraftClient.getInstance().getWindow().getHandle(), ((DuckKeyBinding) keyBinding).getKey().getCode()) == 1 : keyBinding.isPressed();
    }
}
