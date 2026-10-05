package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.mojang.blaze3d.systems.RenderSystem;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.CrosshairHelper;
import me.mioclient.MatrixStackEvent_2;
import me.mioclient.SearchHelper_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.module.client.HUD;
import me.mioclient.module.combat.NoHitDelay;
import me.mioclient.module.render.Crosshair;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.hud.InGameHud;
import net.minecraft.client.gui.screen.ChatScreen;
import net.minecraft.client.option.AttackIndicator;
import net.minecraft.client.render.RenderTickCounter;
import net.minecraft.scoreboard.ScoreboardObjective;
import org.lwjgl.opengl.GL11;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({InGameHud.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinInGameHud.class */
public class MixinInGameHud implements SearchHelper_4 {
    private static HUD hud = (HUD) BaritoneHelper_3.baritoneHelper_4.getModule117(HUD.class);
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static Crosshair crosshair = (Crosshair) BaritoneHelper_3.baritoneHelper_4.getModule117(Crosshair.class);
    private static NoHitDelay nohitdelay = (NoHitDelay) BaritoneHelper_3.baritoneHelper_4.getModule117(NoHitDelay.class);

    @Inject(method = {"renderPlayerList"}, at = {@At("HEAD")})
    public void render(DrawContext drawContext, RenderTickCounter renderTickCounter, CallbackInfo callbackInfo) {
        hud.do736(minecraftClient.currentScreen instanceof ChatScreen ? 12.0f : 0.0f);
        if (MinecraftClient.getInstance().inGameHud.getDebugHud().shouldShowDebugHud() || MinecraftClient.getInstance().options.hudHidden) {
            return;
        }
        RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
        drawContext.fill(-1, -1, 0, 0, 0);
        RenderSystem.disableDepthTest();
        RenderSystem.enableBlend();
        RenderSystem.blendFunc(770, 771);
        RenderSystem.disableCull();
        GL11.glEnable(2848);
        baritoneHelper.getObject1794(MatrixStackEvent_2.getMatrixStackEvent_21234(drawContext.getMatrices(), drawContext, SearchHelper_2.get536()));
        CrosshairHelper.do1597();
        RenderSystem.enableDepthTest();
        GL11.glDisable(2848);
    }

    @Inject(method = {"renderHeldItemTooltip(Lnet/minecraft/client/gui/DrawContext;)V"}, at = {@At("HEAD")}, cancellable = true)
    private void renderHeldItemTooltipHook(DrawContext drawContext, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.heldTooltips.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"renderStatusEffectOverlay"}, at = {@At("HEAD")}, cancellable = true)
    private void renderStatusEffectOverlayHook(DrawContext drawContext, RenderTickCounter renderTickCounter, CallbackInfo callbackInfo) {
        if (hud.isToggled() && hud.setting2.getValue() == HUD.HUDMode.HIDE) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"renderCrosshair"}, at = {@At("HEAD")}, cancellable = true)
    private void onRenderCrosshair(DrawContext drawContext, RenderTickCounter renderTickCounter, CallbackInfo callbackInfo) {
        if (crosshair.isToggled()) {
            callbackInfo.cancel();
        }
    }

    @ModifyExpressionValue(method = {"renderCrosshair"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/option/SimpleOption;getValue()Ljava/lang/Object;", ordinal = 1)})
    public <T> Object renderAttackIndicatorHook(T t) {
        return nohitdelay.isToggled() ? AttackIndicator.OFF : t;
    }

    @Inject(method = {"renderScoreboardSidebar(Lnet/minecraft/client/gui/DrawContext;Lnet/minecraft/scoreboard/ScoreboardObjective;)V"}, at = {@At("HEAD")}, cancellable = true)
    private void renderScoreboardSidebarHook(DrawContext drawContext, ScoreboardObjective scoreboardObjective, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.scoreBoard.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
        drawContext.getMatrices().push();
        drawContext.getMatrices().translate(-Math.max(hud.get737() - 1, 0), 0.0f, 0.0f);
    }

    @Inject(method = {"renderScoreboardSidebar(Lnet/minecraft/client/gui/DrawContext;Lnet/minecraft/scoreboard/ScoreboardObjective;)V"}, at = {@At("TAIL")}, cancellable = true)
    private void nigger(DrawContext drawContext, ScoreboardObjective scoreboardObjective, CallbackInfo callbackInfo) {
        drawContext.getMatrices().pop();
    }

    @ModifyArgs(method = {"renderMiscOverlays"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/hud/InGameHud;renderOverlay(Lnet/minecraft/client/gui/DrawContext;Lnet/minecraft/util/Identifier;F)V", ordinal = 0))
    private void renderOverlayHook(Args args) {
        if (norender.isToggled() && norender.blindness.getValue().booleanValue()) {
            args.set(2, Float.valueOf(0.0f));
        }
    }
}
