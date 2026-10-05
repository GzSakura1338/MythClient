package today.starain.mixin;

import net.minecraft.client.GuiMessage;
import net.minecraft.client.Minecraft;
import net.minecraft.client.GuiMessageTag;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.ChatComponent;
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.MessageSignature;
import net.minecraft.client.gui.components.ComponentRenderUtils;
import net.minecraft.util.FormattedCharSequence;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import today.starain.interfaces.ChatComponentDuck;
import today.starain.module.impl.client.HUD;
import today.starain.utils.client.ChatFlow;
import today.starain.utils.client.ChatHudRenderer;

import java.util.List;

@Mixin(ChatComponent.class)
public class MixinChatComponent implements ChatComponentDuck {

    @Shadow
    private int chatScrollbarPos;

    @Shadow
    @Final
    private List<GuiMessage.Line> trimmedMessages;

    @Override
    public List<GuiMessage.Line> starain$lines() {
        return trimmedMessages;
    }

    @Override
    public int starain$scroll() {
        return chatScrollbarPos;
    }

    @Unique
    private float starain$scrollAnim;
    @Unique
    private float starain$messageAnim;
    @Unique
    private int starain$scrollBase;
    @Unique
    private int starain$extraLines;
    @Unique
    private long starain$lastNano;
    @Unique
    private boolean starain$shifted;

    @Redirect(
            method = "render",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/GuiGraphics;fill(IIIII)V")
    )
    private void starain$chatFill(GuiGraphics guiGraphics, int x1, int y1, int x2, int y2, int color) {
        if (HUD.isChatBackgroundEnabled() || Math.abs(x2 - x1) <= 8 || Math.abs(y2 - y1) < 8) {
            guiGraphics.fill(x1, y1, x2, y2, color);
        }
    }

    @Redirect(
            method = "render",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/GuiGraphics;drawString(Lnet/minecraft/client/gui/Font;Lnet/minecraft/util/FormattedCharSequence;III)I")
    )
    private int starain$flowPrefix(net.minecraft.client.gui.GuiGraphics guiGraphics, net.minecraft.client.gui.Font font,
                                   FormattedCharSequence text, int x, int y, int color) {
        return guiGraphics.drawString(font, ChatFlow.recolor(text), x, y, color);
    }

    @Inject(
            method = "addMessage(Lnet/minecraft/network/chat/Component;Lnet/minecraft/network/chat/MessageSignature;ILnet/minecraft/client/GuiMessageTag;Z)V",
            at = @At("TAIL")
    )
    private void starain$onMessage(Component message, MessageSignature signature, int addedTime,
                                   GuiMessageTag tag, boolean onlyTrim, CallbackInfo ci) {
        if (onlyTrim) {
            return;
        }
        if (ChatHudRenderer.isActive()) {
            ChatHudRenderer.onMessage(ComponentRenderUtils.wrapComponents(message,
                    ChatHudRenderer.wrapLineWidth(), Minecraft.getInstance().font).size());
            return;
        }
        if (chatScrollbarPos == 0 && HUD.isSmoothChatEnabled()) {
            starain$messageAnim = 1.0f;
        }
    }

    @Inject(method = "render", at = @At("HEAD"), cancellable = true)
    private void starain$smoothPush(GuiGraphics guiGraphics, int tickCount, int mouseX, int mouseY, CallbackInfo ci) {
        if (ChatHudRenderer.isActive()) {
            ChatHudRenderer.render((ChatComponent) (Object) this, guiGraphics, tickCount);
            ci.cancel();
            return;
        }
        starain$scrollBase = chatScrollbarPos;
        starain$extraLines = 0;
        if (!HUD.isSmoothChatEnabled()) {
            starain$scrollAnim = chatScrollbarPos;
            starain$messageAnim = 0.0f;
            starain$lastNano = 0L;
            return;
        }

        long now = System.nanoTime();
        float delta = starain$lastNano == 0L ? 0.0f : Math.min(0.05f, (now - starain$lastNano) / 1.0E9f);
        starain$lastNano = now;

        float lines = ((ChatComponent) (Object) this).getLinesPerPage();
        if (Math.abs(chatScrollbarPos - starain$scrollAnim) > lines) {
            starain$scrollAnim = chatScrollbarPos;
        }
        float factor = 1.0f - (float) Math.exp(-delta * 14.0d);
        starain$scrollAnim += (chatScrollbarPos - starain$scrollAnim) * factor;
        starain$messageAnim -= starain$messageAnim * factor;

        starain$scrollBase = (int) Math.floor(starain$scrollAnim);
        float shift = starain$scrollAnim - starain$scrollBase + starain$messageAnim;
        if (shift < 0.002f) {
            return;
        }

        starain$extraLines = (int) Math.ceil(shift);
        starain$shifted = true;
        guiGraphics.pose().pushPose();
        guiGraphics.pose().translate(0.0f, shift * starain$lineHeight(), 0.0f);
    }

    @Inject(method = "render", at = @At("RETURN"))
    private void starain$smoothPop(GuiGraphics guiGraphics, int tickCount, int mouseX, int mouseY, CallbackInfo ci) {
        if (starain$shifted) {
            starain$shifted = false;
            guiGraphics.pose().popPose();
        }
    }

    @Redirect(
            method = "render",
            at = @At(value = "FIELD", target = "Lnet/minecraft/client/gui/components/ChatComponent;chatScrollbarPos:I")
    )
    private int starain$smoothScrollPos(ChatComponent self) {
        return starain$scrollBase;
    }

    @Redirect(
            method = "render",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/components/ChatComponent;getLinesPerPage()I")
    )
    private int starain$linesPerPage(ChatComponent self) {
        return self.getLinesPerPage() + starain$extraLines;
    }

    @Redirect(
            method = "addMessage(Lnet/minecraft/network/chat/Component;Lnet/minecraft/network/chat/MessageSignature;ILnet/minecraft/client/GuiMessageTag;Z)V",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/components/ChatComponent;getWidth()I")
    )
    private int starain$wrapWidth(ChatComponent self) {
        return ChatHudRenderer.isActive() ? ChatHudRenderer.wrapWidth(self) : self.getWidth();
    }

    @Inject(method = "screenToChatX", at = @At("HEAD"), cancellable = true)
    private void starain$hitX(double x, CallbackInfoReturnable<Double> cir) {
        if (!ChatHudRenderer.isActive()) {
            return;
        }
        double scale = ((ChatComponent) (Object) this).getScale();
        cir.setReturnValue(x / scale - 4.0d - ChatHudRenderer.MARGIN / scale);
    }

    @Inject(method = "screenToChatY", at = @At("HEAD"), cancellable = true)
    private void starain$hitY(double y, CallbackInfoReturnable<Double> cir) {
        if (!ChatHudRenderer.isActive()) {
            return;
        }
        double scale = ((ChatComponent) (Object) this).getScale();
        double spacing = Minecraft.getInstance().options.chatLineSpacing().get();
        double lines = (int) (9.0d * (spacing + 1.0d));
        double distance = Minecraft.getInstance().getWindow().getGuiScaledHeight() - y - 40.0d
                - ChatHudRenderer.MARGIN;
        cir.setReturnValue(distance / (scale * lines));
    }

    @Redirect(
            method = "getMessageLineIndexAt",
            at = @At(value = "FIELD", target = "Lnet/minecraft/client/gui/components/ChatComponent;chatScrollbarPos:I")
    )
    private int starain$hitScroll(ChatComponent self) {
        return ChatHudRenderer.isActive() ? ChatHudRenderer.scrollBase() : chatScrollbarPos;
    }

    @Redirect(
            method = "getMessageLineIndexAt",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/components/ChatComponent;getLinesPerPage()I")
    )
    private int starain$hitRows(ChatComponent self) {
        return ChatHudRenderer.isActive() ? ChatHudRenderer.visibleRows() : self.getLinesPerPage();
    }

    @Unique
    private float starain$lineHeight() {
        double spacing = Minecraft.getInstance().options.chatLineSpacing().get();
        return (float) (9.0d * (spacing + 1.0d) * ((ChatComponent) (Object) this).getScale());
    }
}
