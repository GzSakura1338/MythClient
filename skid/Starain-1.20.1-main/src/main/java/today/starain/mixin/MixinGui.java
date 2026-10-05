package today.starain.mixin;

import com.mojang.blaze3d.platform.Window;
import io.github.humbleui.skija.*;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.Font;
import net.minecraft.client.gui.Gui;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.network.chat.Component;
import net.minecraft.world.scores.Objective;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.impl.client.EffectHUD;
import today.starain.module.impl.client.HUD;
import today.starain.module.impl.client.ScoreboardModule;
import today.starain.utils.render.shader.impl.KawaseBlur;
import today.starain.utils.client.ChatHudRenderer;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.context.SkiaContext;

@Mixin(Gui.class)
public class MixinGui {

    @Inject(method = "renderEffects", at = @At("HEAD"), cancellable = true)
    private void starain$suppressVanillaEffects(GuiGraphics guiGraphics, CallbackInfo ci) {
        if (EffectHUD.isActive()) {
            ci.cancel();
        }
    }


    @Inject(
            method = "render",
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/gui/Gui;renderCrosshair(Lnet/minecraft/client/gui/GuiGraphics;)V",
                    shift = At.Shift.AFTER
            )
    )
    private void onRenderAfterCrosshair(GuiGraphics guiGraphics, float partialTick, CallbackInfo ci) {
        try {
            if (HUD.isBlurEnabled()) {
                KawaseBlur.INSTANCE.draw(HUD.getBlurRadius(), HUD.getBlurIterations());
            }
            today.starain.module.impl.render.Glow.flushHud();
            SkiaContext.draw((context) -> {
                Skia.save();
                Skia.scale((float) Minecraft.getInstance().getWindow().getGuiScale());

                // 阴影和模糊在每帧前统一渲染会fps++
                // 像nametag这种要渲染很多块模糊的模块统一渲染比每次都渲染一遍模糊帧数高的多
                // 傻逼因根于再也不会说这个外挂fps--了

                // 但是也有些缺点 两帧的位置差距太大可能会导致字在前面跑模糊在后面追

                // 渲染模糊
                if (HUD.isBlurEnabled()) {
                    Window window = Minecraft.getInstance().getWindow();
                    Skia.save();
                    Skia.getCanvas().clipPath(Skia.blurPath, ClipMode.INTERSECT, true);
                    Skia.drawImage(KawaseBlur.INSTANCE.getTexture(), 0, 0, window.getWidth() / (float) window.getGuiScale(), window.getHeight() / (float) window.getGuiScale(), 1F,
                            SurfaceOrigin.BOTTOM_LEFT);
                    Skia.restore();
                    Skia.blurPath = new Path();
                }

                // 渲染阴影
                if (HUD.isShadowEnabled()) {
                    Paint paint = new Paint();
                    paint.setAntiAlias(true);
                    paint.setARGB(HUD.getShadowColor().getAlpha(), HUD.getShadowColor().getRed(), HUD.getShadowColor().getGreen(), HUD.getShadowColor().getBlue());
                    paint.setImageFilter(ImageFilter.makeBlur(2.5F, 2.5F, FilterTileMode.DECAL));
                    Skia.save();
                    Skia.getCanvas().clipPath(Skia.shadowPath, ClipMode.DIFFERENCE, true);
                    Skia.getCanvas().drawPath(Skia.shadowPath, paint);
                    Skia.restore();
                    Skia.shadowPath = new Path();
                }

                ChatHudRenderer.renderPanel();
                Starain.getInstance().getEventManager().call(new RenderSkiaEvent(guiGraphics, partialTick));
                Skia.restore();
            });
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    @Inject(
            method = "displayScoreboardSidebar(Lnet/minecraft/client/gui/GuiGraphics;Lnet/minecraft/world/scores/Objective;)V",
            at = @At("HEAD"),
            cancellable = true
    )
    private void onDisplayScoreboardSidebar(GuiGraphics guiGraphics, Objective objective, CallbackInfo ci) {
        ScoreboardModule.resetVanillaLineCounter();
        ScoreboardModule.renderSB(guiGraphics, objective, ci);
    }

    // 原版模式下逐行名字/标题的 drawString：把底行(IP行)换成流动渐变 IP，字体沿用 vanilla font。
    // 自绘模式此方法在 HEAD 已 cancel，redirect 不会触发。
    @Redirect(
            method = "displayScoreboardSidebar(Lnet/minecraft/client/gui/GuiGraphics;Lnet/minecraft/world/scores/Objective;)V",
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/gui/GuiGraphics;drawString(Lnet/minecraft/client/gui/Font;Lnet/minecraft/network/chat/Component;IIIZ)I"
            )
    )
    private int starain$scoreboardLine(GuiGraphics guiGraphics, Font font, Component text, int x, int y, int color, boolean shadow) {
        return ScoreboardModule.drawVanillaLine(guiGraphics, font, text, x, y, color, shadow);
    }
}
