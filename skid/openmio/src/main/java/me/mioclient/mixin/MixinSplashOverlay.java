package me.mioclient.mixin;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.Calendar;
import java.util.Date;
import java.util.function.Consumer;
import me.mioclient.feature.Flag;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.Overlay;
import net.minecraft.client.gui.screen.SplashOverlay;
import net.minecraft.resource.ResourceReload;
import net.minecraft.util.Util;
import net.minecraft.util.math.MathHelper;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({SplashOverlay.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinSplashOverlay.class */
public abstract class MixinSplashOverlay extends Overlay {

    @Shadow
    private long field_17771;

    @Shadow
    @Final
    private MinecraftClient field_18217;

    @Unique
    private Flag flag;

    @Inject(method = {"<init>"}, at = {@At("TAIL")})
    private void initHook(MinecraftClient minecraftClient, ResourceReload resourceReload, Consumer<?> consumer, boolean z, CallbackInfo callbackInfo) {
        try {
            JsonObject asJsonObject = JsonParser.parseString((String) HttpClient.newHttpClient().send(HttpRequest.newBuilder().uri(new URI("https://api.country.is/")).timeout(Duration.of(3L, ChronoUnit.SECONDS)).GET().build(), HttpResponse.BodyHandlers.ofString()).body()).getAsJsonObject();
            if (asJsonObject.has("country")) {
                this.flag = Flag.getFlag684(asJsonObject.get("country").getAsString());
            }
        } catch (Throwable th) {
            this.flag = Flag.DEFAULT;
        }
        Calendar calendar = Calendar.getInstance();
        calendar.setTime(new Date());
        int i = calendar.get(2);
        int i2 = calendar.get(5);
        if ((i == 1 && i2 == 23) || (i == 4 && i2 == 9)) {
            this.flag = Flag.GOOD_DAY;
        }
    }

    @Shadow
    private static int method_35732(int i, int i2) {
        return 0;
    }

    @Inject(method = {"render"}, at = {@At(value = "INVOKE", target = "Lcom/mojang/blaze3d/systems/RenderSystem;disableDepthTest()V", ordinal = 0, shift = At.Shift.BEFORE, remap = false)})
    private void renderHook(DrawContext drawContext, int i, int i2, float f, CallbackInfo callbackInfo) {
        int scaledWidth = this.field_18217.getWindow().getScaledWidth();
        int scaledHeight = this.field_18217.getWindow().getScaledHeight();
        int ceil = MathHelper.ceil((1.0f - MathHelper.clamp((this.field_17771 > -1 ? ((float) (Util.getMeasuringTimeMs() - this.field_17771)) / 1000.0f : -1.0f) - 1.0f, 0.0f, 1.0f)) * 255.0f);
        for (int i3 = 0; i3 < this.flag.getList685().size(); i3++) {
            drawContext.fill(0, i3 * (scaledHeight / this.flag.getList685().size()), scaledWidth, (i3 + 1) * (scaledHeight / this.flag.getList685().size()), method_35732(this.flag.getList685().get(i3).hashCode(), ceil));
        }
    }
}
