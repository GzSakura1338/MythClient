package today.starain.mixin;

import com.mojang.blaze3d.platform.GlStateManager;
import net.minecraft.Util;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.screens.LoadingOverlay;
import net.minecraft.client.renderer.RenderType;
import net.minecraft.server.packs.resources.ReloadInstance;
import net.minecraft.util.Mth;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.ui.loading.HelloAnimation;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.context.SkiaContext;

import java.util.Optional;
import java.util.function.Consumer;

@Mixin(LoadingOverlay.class)
public abstract class MixinLoadingOverlay {

    @Shadow
    @Final
    private Minecraft minecraft;

    @Shadow
    @Final
    private ReloadInstance reload;

    @Shadow
    @Final
    private Consumer<Optional<Throwable>> onFinish;

    @Shadow
    @Final
    private boolean fadeIn;

    @Shadow
    private float currentProgress;

    @Shadow
    private long fadeOutStart;

    @Shadow
    private long fadeInStart;

    @Unique
    private long starain$begin = -1L;

    @Inject(method = "render", at = @At("HEAD"), cancellable = true)
    private void starain$renderHello(GuiGraphics graphics, int mouseX, int mouseY, float partialTick, CallbackInfo ci) {
        ci.cancel();

        int width = graphics.guiWidth();
        int height = graphics.guiHeight();
        long now = Util.getMillis();

        if (starain$begin == -1L) {
            starain$begin = now;
        }
        if (this.fadeIn && this.fadeInStart == -1L) {
            this.fadeInStart = now;
        }

        float fadeOut = this.fadeOutStart > -1L ? (float) (now - this.fadeOutStart) / 1000.0F : -1.0F;
        float fadeInProgress = this.fadeInStart > -1L ? (float) (now - this.fadeInStart) / 500.0F : -1.0F;

        float alpha;
        if (fadeOut >= 1.0F) {
            if (this.minecraft.screen != null) {
                this.minecraft.screen.render(graphics, 0, 0, partialTick);
            }
            alpha = 1.0F - Mth.clamp(fadeOut - 1.0F, 0.0F, 1.0F);
            graphics.fill(RenderType.guiOverlay(), 0, 0, width, height, Mth.ceil(alpha * 255.0F) << 24);
        } else if (this.fadeIn) {
            if (this.minecraft.screen != null && fadeInProgress < 1.0F) {
                this.minecraft.screen.render(graphics, mouseX, mouseY, partialTick);
            }
            alpha = Mth.clamp(fadeInProgress, 0.0F, 1.0F);
            graphics.fill(RenderType.guiOverlay(), 0, 0, width, height,
                    Mth.ceil(Mth.clamp((double) fadeInProgress, 0.15D, 1.0D) * 255.0D) << 24);
        } else {
            GlStateManager._clearColor(0.0F, 0.0F, 0.0F, 1.0F);
            GlStateManager._clear(16384, Minecraft.ON_OSX);
            alpha = 1.0F;
        }

        this.currentProgress = Mth.clamp(this.currentProgress * 0.95F
                + this.reload.getActualProgress() * 0.050000012F, 0.0F, 1.0F);

        starain$paint(graphics, width, height,
                HelloAnimation.strokeProgress(now - starain$begin), alpha, this.currentProgress);

        if (fadeOut >= 2.0F) {
            this.minecraft.setOverlay(null);
        }

        if (this.fadeOutStart == -1L
                && this.reload.isDone()
                && (!this.fadeIn || fadeInProgress >= 2.0F)
                && now - starain$begin >= HelloAnimation.HOLD_MILLIS) {
            try {
                this.reload.checkExceptions();
                this.onFinish.accept(Optional.empty());
            } catch (Throwable throwable) {
                this.onFinish.accept(Optional.of(throwable));
            }
            this.fadeOutStart = Util.getMillis();
            if (this.minecraft.screen != null) {
                this.minecraft.screen.init(this.minecraft, graphics.guiWidth(), graphics.guiHeight());
            }
        }
    }

    @Unique
    private void starain$paint(GuiGraphics graphics, int width, int height,
                               float progress, float alpha, float load) {
        if (SkiaContext.getContext() != null) {
            try {
                float guiScale = (float) this.minecraft.getWindow().getGuiScale();
                SkiaContext.draw((canvas) -> {
                    Skia.save();
                    Skia.scale(guiScale);
                    HelloAnimation.draw(width, height, progress, alpha, load);
                    Skia.restore();
                });
                return;
            } catch (Throwable ignored) {
            }
        }
        starain$fallback(graphics, width, height, alpha, load);
    }

    @Unique
    private void starain$fallback(GuiGraphics graphics, int width, int height, float alpha, float load) {
        int opacity = Mth.ceil(Mth.clamp(alpha, 0.0F, 1.0F) * 255.0F);
        if (opacity <= 0) {
            return;
        }
        int barWidth = (int) Math.min(width * 0.26F, 210.0F);
        int x = (width - barWidth) / 2;
        int y = (int) (height * 0.79F);
        graphics.fill(x, y, x + barWidth, y + 3, ((opacity / 5) << 24) | 0xFFFFFF);
        graphics.fill(x, y, x + (int) (barWidth * Mth.clamp(load, 0.0F, 1.0F)), y + 3, (opacity << 24) | 0xFFFFFF);
    }
}
