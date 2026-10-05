package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.BlurFramebuffer;
import me.mioclient.ChatFilterSearchHelper4_2;
import me.mioclient.FontsSearchHelper4_2;
import me.mioclient.module.render.Blur;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.text.Style;
import org.jetbrains.annotations.Nullable;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({Screen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinScreen.class */
public abstract class MixinScreen {
    private static final NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static final Blur blur = (Blur) BaritoneHelper_3.baritoneHelper_4.getModule117(Blur.class);

    @Shadow
    @Nullable
    protected MinecraftClient field_22787;

    @Shadow
    public int field_22789;

    @Shadow
    public int field_22790;

    @Shadow
    protected abstract void method_57735(DrawContext drawContext);

    @Inject(method = {"handleTextClick"}, at = {@At(value = "INVOKE", target = "Ljava/lang/String;startsWith(Ljava/lang/String;)Z", shift = At.Shift.BEFORE)})
    private void handleTextClickHook(Style style, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (this.field_22787.getNetworkHandler() == null) {
            return;
        }
        String value = style.getClickEvent().getValue();
        if (value.startsWith(ChatFilterSearchHelper4_2.getString2982())) {
            this.field_22787.getNetworkHandler().sendChatMessage(value);
        }
    }

    @Inject(method = {"renderInGameBackground"}, at = {@At("HEAD")}, cancellable = true)
    private void renderInGameBackgroundHook(DrawContext drawContext, CallbackInfo callbackInfo) {
        if (norender.is1998()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"renderBackground"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/screen/Screen;applyBlur(F)V", shift = At.Shift.BEFORE)}, cancellable = true)
    private void renderBackground(DrawContext drawContext, int i, int i2, float f, CallbackInfo callbackInfo) {
        if (blur.isToggled() && this.field_22787.world != null && this.field_22787.player != null) {
            method_57735(drawContext);
            callbackInfo.cancel();
        } else if (MinecraftClient.getInstance().currentScreen instanceof FontsSearchHelper4_2) {
            float menuBackgroundBlurrinessValue = this.field_22787.options.getMenuBackgroundBlurrinessValue();
            method_57735(drawContext);
            BlurFramebuffer.do2002(() -> {
                drawContext.fill(0, 0, this.field_22789, this.field_22790, -1);
            }, menuBackgroundBlurrinessValue);
            callbackInfo.cancel();
        }
    }
}
