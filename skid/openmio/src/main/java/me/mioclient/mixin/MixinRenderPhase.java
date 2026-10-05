package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.KeyPearlMode;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.EndDrawingEvent;
import me.mioclient.module.render.Chams;
import net.minecraft.client.render.RenderPhase;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({RenderPhase.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinRenderPhase.class */
public class MixinRenderPhase {
    private static final Chams chams = (Chams) BaritoneHelper_3.baritoneHelper_4.getModule117(Chams.class);
    private static final String TEXTURING_ID = "glint_texturing";

    @Unique
    private boolean mio$isGlintTexturing;

    @Inject(method = {"<init>"}, at = {@At("TAIL")})
    private void init(String str, Runnable runnable, Runnable runnable2, CallbackInfo callbackInfo) {
        this.mio$isGlintTexturing = str.contains(TEXTURING_ID);
    }

    @ModifyArgs(method = {"setupGlintTexturing"}, at = @At(value = "INVOKE", target = "Lorg/joml/Matrix4f;translation(FFF)Lorg/joml/Matrix4f;"))
    private static void translateHook(Args args) {
        if (chams.speed.is2327() && Chams.flag2) {
            float intValue = chams.progress.getValue().intValue() / 100.0f;
            args.set(0, Float.valueOf(-intValue));
            args.set(1, Float.valueOf(intValue));
        }
    }

    @Inject(method = {"startDrawing"}, at = {@At("TAIL")})
    private void startDraw(CallbackInfo callbackInfo) {
        if (this.mio$isGlintTexturing) {
            SearchHelper_4.baritoneHelper.getObject1794(new EndDrawingEvent(KeyPearlMode.Pre));
        }
    }

    @Inject(method = {"endDrawing"}, at = {@At("TAIL")})
    private void endDrawing(CallbackInfo callbackInfo) {
        if (this.mio$isGlintTexturing) {
            SearchHelper_4.baritoneHelper.getObject1794(new EndDrawingEvent(KeyPearlMode.Post));
        }
    }
}
