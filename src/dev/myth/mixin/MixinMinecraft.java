package dev.myth.mixin;

import dev.myth.Myth;
import dev.myth.access.MinecraftAccess;
import dev.myth.event.api.EventType;
import dev.myth.event.impl.ClickEvent;
import dev.myth.event.impl.GameTickEvent;
import net.minecraft.client.Minecraft;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(Minecraft.class)
public abstract class MixinMinecraft implements MinecraftAccess {
    @Invoker("pick")
    public abstract void myth$pick(float partialTicks);

    @Inject(method = "renderFrame", at = @At(value = "INVOKE", target = "Lcom/mojang/blaze3d/systems/GpuSurface;present()V"))
    private void myth$renderImGui(boolean advanceGameTime, CallbackInfo ci) {
        Myth.MODEL_MANAGER.updateHeldKeybinds(Minecraft.getInstance());
        Myth.UI_MANAGER.render();
    }

    @Inject(method = "tick", at = @At("HEAD"))
    private void myth$tickPre(CallbackInfo ci) {
        Myth.EVENT_MANAGER.call(new GameTickEvent(EventType.PRE));
    }

    @Inject(method = "tick", at = @At("RETURN"))
    private void myth$tickPost(CallbackInfo ci) {
        Myth.EVENT_MANAGER.call(new GameTickEvent(EventType.POST));
    }

    @Inject(method = "handleKeybinds", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;isUsingItem()Z", ordinal = 0), cancellable = true)
    private void myth$click(CallbackInfo ci) {
        if (Myth.EVENT_MANAGER.call(new ClickEvent()).isCancelled()) {
            ci.cancel();
        }
    }
}
