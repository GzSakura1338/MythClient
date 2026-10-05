package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.v2.WrapWithCondition;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import com.llamalad7.mixinextras.sugar.Local;
import java.util.ConcurrentModificationException;
import java.util.List;
import me.mioclient.AdvanceGlyph;
import me.mioclient.AntiSpamHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ChatFilterSearchHelper4_2;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.SearchHelper_4;
import me.mioclient.SignatureHelper;
import me.mioclient.event.AddMessageEvent;
import me.mioclient.module.Chat;
import me.mioclient.module.client.Fonts;
import me.mioclient.module.misc.BetterChat;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.font.TextHandler;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.hud.ChatHud;
import net.minecraft.client.gui.hud.ChatHudLine;
import net.minecraft.client.gui.hud.MessageIndicator;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.network.message.MessageSignatureData;
import net.minecraft.text.OrderedText;
import net.minecraft.text.Text;
import net.minecraft.util.math.MathHelper;
import org.jetbrains.annotations.Nullable;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArg;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({ChatHud.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinChatHud.class */
public abstract class MixinChatHud implements SearchHelper_4, AntiSpamHelper {
    private static final NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static final BetterChat betterchat = (BetterChat) BaritoneHelper_3.baritoneHelper_4.getModule117(BetterChat.class);
    private static final Chat chathud = (Chat) BaritoneHelper_3.baritoneHelper_4.getModule117(Chat.class);
    private static final Fonts fonts = (Fonts) BaritoneHelper_3.baritoneHelper_4.getModule117(Fonts.class);

    @Shadow
    @Final
    private List<ChatHudLine.Visible> field_2064;

    @Unique
    private MessageSignatureData last;

    @Unique
    private int current;

    @Unique
    private static boolean skip;

    @Shadow
    protected abstract void method_45027(ChatHudLine chatHudLine);

    @Shadow
    protected abstract void method_1815(ChatHudLine chatHudLine);

    @Shadow
    protected abstract void method_58744(ChatHudLine chatHudLine);

    @Shadow
    public abstract void method_44811(Text text, @Nullable MessageSignatureData messageSignatureData, @Nullable MessageIndicator messageIndicator);

    @Shadow
    public abstract double method_1814();

    @Shadow
    protected abstract int method_44752();

    private void drawIndicatorIcon(MatrixStack matrixStack, int i, int i2, MessageIndicator.Icon icon) {
    }

    @Inject(method = {"logChatMessage"}, at = {@At("HEAD")}, cancellable = true)
    private void logChatMessageHook(ChatHudLine chatHudLine, CallbackInfo callbackInfo) {
        if (chatHudLine.indicator() == MixinMessageIndicatorHelper.messageIndicator) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"addVisibleMessage"}, at = {@At("HEAD")})
    private void addMessage(ChatHudLine chatHudLine, CallbackInfo callbackInfo) {
        BetterChat.flag = true;
        MessageSignatureData signature = chatHudLine.signature();
        this.last = signature;
        if (signature == null || signature.toByteBuffer().getInt() >= 0) {
            return;
        }
        try {
            minecraftClient.inGameHud.getChatHud().visibleMessages.removeIf(visible -> {
                return ((visible == null || ((SignatureHelper)(Object) visible).getSignature() == null || !((SignatureHelper)(Object) visible).getSignature().equals(signature)) ? false : true);
            });
        } catch (ConcurrentModificationException e) {
        }
    }

    @Inject(method = {"addVisibleMessage"}, at = {@At("RETURN")})
    private void noracismcattyn(ChatHudLine chatHudLine, CallbackInfo callbackInfo) {
        BetterChat.flag = false;
    }

    @Redirect(method = {"addVisibleMessage"}, at = @At(value = "INVOKE", target = "Ljava/util/List;add(ILjava/lang/Object;)V", ordinal = 0))
    private <E> void visibleInitHook(List<E> list, int i, E e) {
        ((SignatureHelper) e).setSignature(this.last);
        SearchHelper_4.baritoneHelper.getObject1794(new AddMessageEvent((ChatHudLine.Visible) e));
        list.add(i, e);
    }

    @Redirect(method = {"render"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/hud/ChatHudLine$Visible;indicator()Lnet/minecraft/client/gui/hud/MessageIndicator;"), require = 0)
    private MessageIndicator renderHook(ChatHudLine.Visible visible) {
        if (norender.isToggled() && norender.messageIndicator.getValue().booleanValue()) {
            return null;
        }
        return visible.indicator();
    }

    @WrapWithCondition(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/DrawContext;fill(IIIII)V", ordinal = 1)})
    private boolean renderHook2(DrawContext drawContext, int i, int i2, int i3, int i4, int i5) {
        return (norender.isToggled() && norender.messageIndicator.getValue().booleanValue()) ? false : true;
    }

    @WrapOperation(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/DrawContext;drawTextWithShadow(Lnet/minecraft/client/font/TextRenderer;Lnet/minecraft/text/OrderedText;III)I")})
    private int textHook(DrawContext drawContext, TextRenderer textRenderer, OrderedText orderedText, int i, int i2, int i3, Operation<Integer> operation) {
        int i4 = i - ((norender.isToggled() && norender.messageIndicator.getValue().booleanValue()) ? 3 : 0);
        if (betterchat.is677() && this.current < this.field_2064.size()) {
            SignatureHelper signatureHelper = (SignatureHelper)(Object)(this.field_2064.get(this.current));
            signatureHelper.getProgress().do2139(true);
            float f = signatureHelper.getProgress().get172();
            long mio$getAddTime = signatureHelper.mio$getAddTime();
            boolean z = betterchat.type.getValue() == BetterChat.BetterChatMode_2.BOTH;
            if (betterchat.type.getValue() == BetterChat.BetterChatMode_2.VERTICAL || z) {
                i2 = (int) (i2 + (9.0f * (1.0f - f)));
            }
            if (betterchat.type.getValue() == BetterChat.BetterChatMode_2.HORIZONTAL || z) {
                i4 = (int) (i4 - (textRenderer.getWidth(orderedText) * (1.0f - f)));
            }
            if (betterchat.alpha.getValue().intValue() != 0) {
                i3 = (i3 & 16777215) | (((int) Math.max(((i3 >> 24) & 255) * MathHelper.clamp(((float) (System.currentTimeMillis() - mio$getAddTime)) / betterchat.alpha.getValue().intValue(), 0.0f, 1.0f), 10.0f)) << 24);
            }
        }
        AdvanceGlyph advanceGlyph1686 = FontsSearchHelper4.fontsSearchHelper4.getAdvanceGlyph1686();
        if (advanceGlyph1686 != null && fonts.isToggled() && fonts.chat.getValue().booleanValue()) {
            advanceGlyph1686.get2765(drawContext.getMatrices(), orderedText, i4, i2, i3, true);
            return 0;
        }
        operation.call(new Object[]{drawContext, textRenderer, orderedText, Integer.valueOf(i4), Integer.valueOf(i2), Integer.valueOf(i3)});
        return 0;
    }

    @Inject(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;translate(FFF)V", ordinal = 0)})
    private void renderPreHook(DrawContext drawContext, int i, int i2, int i3, boolean z, CallbackInfo callbackInfo) {
        float f = 0.0f;
        float f2 = 0.0f;
        if (chathud.isToggled()) {
            f = 0.0f + chathud.get988();
            f2 = 0.0f + chathud.get987();
        }
        if (betterchat.is677() && betterchat.type.getValue() == BetterChat.BetterChatMode_2.BOUNCE) {
            f2 += (betterchat.progress.get2138() - 1.0f) * 8.0f;
        }
        if (f == 0.0f && f2 == 0.0f) {
            return;
        }
        drawContext.getMatrices().translate(f, f2, 0.0f);
    }

    @Inject(method = {"render"}, at = {@At("TAIL")})
    private void render(DrawContext drawContext, int i, int i2, int i3, boolean z, CallbackInfo callbackInfo) {
        FontsSearchHelper4.fontsSearchHelper4.do1597();
    }

    @ModifyArg(method = {"render"}, at = @At(value = "INVOKE", target = "Ljava/util/List;get(I)Ljava/lang/Object;", ordinal = 0))
    private int renderHook4(int i) {
        this.current = i;
        return i;
    }

    @Inject(method = {"addMessage(Lnet/minecraft/text/Text;Lnet/minecraft/network/message/MessageSignatureData;Lnet/minecraft/client/gui/hud/MessageIndicator;)V"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/hud/ChatHud;logChatMessage(Lnet/minecraft/client/gui/hud/ChatHudLine;)V", shift = At.Shift.BEFORE)}, cancellable = true)
    private void addMessageHook(Text text, MessageSignatureData messageSignatureData, MessageIndicator messageIndicator, CallbackInfo callbackInfo, @Local ChatHudLine chatHudLine) {
        if (skip) {
            return;
        }
        AddMessageEvent addMessageEvent = new AddMessageEvent(messageIndicator, messageSignatureData, text);
        SearchHelper_4.baritoneHelper.getObject1794(addMessageEvent);
        Text text2279 = addMessageEvent.getText2279();
        MessageSignatureData signature = addMessageEvent.getSignature();
        callbackInfo.cancel();
        if (addMessageEvent.is2403()) {
            return;
        }
        if (text.equals(text2279)) {
            method_45027(chatHudLine);
            method_1815(chatHudLine);
            method_58744(chatHudLine);
        } else {
            skip = true;
            method_44811(text2279, signature, messageIndicator);
            skip = false;
        }
    }

    @ModifyExpressionValue(method = {"addVisibleMessage"}, at = {@At(value = "INVOKE", target = "Ljava/util/List;size()I", ordinal = 2)})
    public int chatHistory(int i) {
        if (betterchat.isToggled() && betterchat.longChatHistory.getValue().booleanValue()) {
            return 1;
        }
        return i;
    }

    @ModifyExpressionValue(method = {"addMessage(Lnet/minecraft/client/gui/hud/ChatHudLine;)V"}, at = {@At(value = "INVOKE", target = "Ljava/util/List;size()I", ordinal = 0)})
    public int chatHistory2(int i) {
        if (betterchat.isToggled() && betterchat.longChatHistory.getValue().booleanValue()) {
            return 1;
        }
        return i;
    }

    @Inject(method = {"clear"}, at = {@At("HEAD")}, cancellable = true)
    private void clearHook(boolean z, CallbackInfo callbackInfo) {
        if (betterchat.isToggled() && betterchat.noReset.getValue().booleanValue() && !BetterChat.flag2) {
            callbackInfo.cancel();
        }
    }

    @WrapOperation(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/DrawContext;fill(IIIII)V")})
    private void fillHook(DrawContext drawContext, int i, int i2, int i3, int i4, int i5, Operation<Void> operation) {
        operation.call(new Object[]{drawContext, Integer.valueOf(i), Integer.valueOf(i2), Integer.valueOf(i3), Integer.valueOf(i4), Integer.valueOf(i5)});
    }

    @WrapOperation(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/DrawContext;fill(IIIIII)V")})
    private void fillHook(DrawContext drawContext, int i, int i2, int i3, int i4, int i5, int i6, Operation<Void> operation) {
        operation.call(new Object[]{drawContext, Integer.valueOf(i), Integer.valueOf(i2), Integer.valueOf(i3), Integer.valueOf(i4), Integer.valueOf(i5), Integer.valueOf(i6)});
    }

    @ModifyExpressionValue(method = {"addToMessageHistory"}, at = {@At(value = "INVOKE", target = "Ljava/lang/String;startsWith(Ljava/lang/String;)Z")})
    private boolean addToMessageHistory(boolean z, @Local(argsOnly = true) String str) {
        return z || str.startsWith(ChatFilterSearchHelper4_2.getString2982());
    }

    @ModifyExpressionValue(method = {"getTextStyleAt"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/font/TextRenderer;getTextHandler()Lnet/minecraft/client/font/TextHandler;")})
    private TextHandler getTextStyleAtHook(TextHandler textHandler) {
        AdvanceGlyph advanceGlyph1686 = FontsSearchHelper4.fontsSearchHelper4.getAdvanceGlyph1686();
        return (advanceGlyph1686 != null && fonts.isToggled() && fonts.chat.getValue().booleanValue()) ? advanceGlyph1686.getTextHandler2776() : textHandler;
    }

    @Inject(method = {"toChatLineX"}, at = {@At("HEAD")}, cancellable = true)
    private void toChatLineX(double d, CallbackInfoReturnable<Double> callbackInfoReturnable) {
        if (!chathud.isToggled() || chathud.get988() == 0.0f) {
            return;
        }
        callbackInfoReturnable.setReturnValue(Double.valueOf((d - chathud.getModuleListSearchHelper43020().get123()) / method_1814()));
    }

    @Inject(method = {"toChatLineY"}, at = {@At("HEAD")}, cancellable = true)
    private void toChatLineY(double d, CallbackInfoReturnable<Double> callbackInfoReturnable) {
        if (!chathud.isToggled() || chathud.get987() == 0.0f) {
            return;
        }
        callbackInfoReturnable.setReturnValue(Double.valueOf(((chathud.getModuleListSearchHelper43020().get124() - d) + chathud.getModuleListSearchHelper43020().getFloatArray2950()[1]) / (method_1814() * method_44752())));
    }

    @Override // me.mioclient.AntiSpamHelper
    public List<ChatHudLine.Visible> getVisible() {
        return this.field_2064;
    }
}
