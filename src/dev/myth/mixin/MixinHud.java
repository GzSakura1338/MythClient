package dev.myth.mixin;

import dev.myth.Myth;
import dev.myth.event.impl.RenderSkiaEvent;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.Hud;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Hud.class)
public abstract class MixinHud {
    @Inject(method = "extractRenderState", at = @At("RETURN"))
    private void myth$renderHud(GuiGraphicsExtractor graphics, DeltaTracker delta, CallbackInfo ci) {
        graphics.pose().pushMatrix();
        try {
            Myth.EVENT_MANAGER.call(new RenderSkiaEvent(graphics, delta.getGameTimeDeltaPartialTick(false)));
        } finally {
            graphics.pose().popMatrix();
        }
    }
}
