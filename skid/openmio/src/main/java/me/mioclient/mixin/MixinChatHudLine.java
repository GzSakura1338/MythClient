package me.mioclient.mixin;

import java.util.function.Supplier;
import me.mioclient.SignatureHelper;
import me.mioclient.feature.Progress;
import me.mioclient.module.misc.BetterChat;
import net.minecraft.client.gui.hud.ChatHudLine;
import net.minecraft.client.gui.hud.MessageIndicator;
import net.minecraft.network.message.MessageSignatureData;
import net.minecraft.text.OrderedText;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({ChatHudLine.Visible.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinChatHudLine.class */
public class MixinChatHudLine implements SignatureHelper {

    @Unique
    private Progress progress = null;

    @Unique
    private MessageSignatureData signature;

    @Mutable
    @Shadow
    @Final
    private OrderedText comp_896;

    @Unique
    private long mio$addTime;

    @Inject(method = {"<init>"}, at = {@At("RETURN")})
    private void onInit(int i, OrderedText orderedText, MessageIndicator messageIndicator, boolean z, CallbackInfo callbackInfo) {
        getProgress().do2140(false);
        this.mio$addTime = System.currentTimeMillis();
    }

    @Override // me.mioclient.SignatureHelper
    public MessageSignatureData getSignature() {
        return this.signature;
    }

    @Override // me.mioclient.SignatureHelper
    public void setSignature(MessageSignatureData messageSignatureData) {
        this.signature = messageSignatureData;
    }

    @Override // me.mioclient.SignatureHelper
    public OrderedText getContent() {
        return this.comp_896;
    }

    @Override // me.mioclient.SignatureHelper
    public void setContent(OrderedText orderedText) {
        this.comp_896 = orderedText;
    }

    @Override // me.mioclient.SignatureHelper
    public Progress getProgress() {
        if (this.progress == null) {
            this.progress = new Progress((Supplier<Float>) () -> {
                return Float.valueOf(BetterChat.getBetterChat678().speed.getValue().floatValue() * 2.0f);
            }, true);
        }
        return this.progress;
    }

    @Override // me.mioclient.SignatureHelper
    public long mio$getAddTime() {
        return this.mio$addTime;
    }

    @Override // me.mioclient.SignatureHelper
    public void mio$setAddTime(long j) {
        this.mio$addTime = j;
    }
}
