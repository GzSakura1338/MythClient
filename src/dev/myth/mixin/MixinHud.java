package dev.myth.mixin;

import dev.myth.Myth;
import dev.myth.event.impl.RenderSkiaEvent;
import dev.myth.model.render.NoRender;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.Hud;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Hud.class)
public abstract class MixinHud {
    @Inject(method = "extractBossOverlay", at = @At("HEAD"), cancellable = true)
    private void myth$bossBars(GuiGraphicsExtractor graphics, DeltaTracker delta, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.bossBars.get()) ci.cancel();
    }

    @Inject(method = "extractScoreboardSidebar", at = @At("HEAD"), cancellable = true)
    private void myth$scoreBoard(GuiGraphicsExtractor graphics, DeltaTracker delta, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.scoreBoard.get()) ci.cancel();
    }

    @Inject(method = "extractSelectedItemName", at = @At("HEAD"), cancellable = true)
    private void myth$heldTooltips(GuiGraphicsExtractor graphics, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.heldTooltips.get()) ci.cancel();
    }

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
