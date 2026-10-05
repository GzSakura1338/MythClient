package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.wrapmethod.WrapMethod;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import dev.myth.UI.hud.Chat;
import net.minecraft.client.gui.Font;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.ActiveTextCollector;
import org.joml.Matrix3x2f;
import net.minecraft.client.gui.components.ChatComponent;
import org.spongepowered.asm.mixin.Mixin;

@Mixin(ChatComponent.class)
public abstract class MixinHudChat {
    @WrapMethod(method = "extractRenderState(Lnet/minecraft/client/gui/GuiGraphicsExtractor;Lnet/minecraft/client/gui/Font;IIILnet/minecraft/client/gui/components/ChatComponent$DisplayMode;Z)V")
    private void myth$chatPlacement(GuiGraphicsExtractor graphics, Font font, int ticks, int mouseX, int mouseY,
                                   ChatComponent.DisplayMode mode, boolean cursor, Operation<Void> original) {
        Chat chat = Chat.active();
        if (chat == null) { original.call(graphics, font, ticks, mouseX, mouseY, mode, cursor); return; }
        graphics.pose().pushMatrix();
        try {
            graphics.pose().translate(chat.offsetX(), chat.offsetY());
            original.call(graphics, font, ticks, mouseX, mouseY, mode, cursor);
        } finally { graphics.pose().popMatrix(); }
    }
    @WrapMethod(method = "captureClickableText")
    private void myth$chatClicks(ActiveTextCollector collector, int screenHeight, int ticks, ChatComponent.DisplayMode mode, Operation<Void> original) {
        Chat chat = Chat.active();
        if (chat == null) { original.call(collector, screenHeight, ticks, mode); return; }
        var parameters = collector.defaultParameters();
        try {
            collector.defaultParameters(parameters.withPose(new Matrix3x2f(parameters.pose()).translate(chat.offsetX(), chat.offsetY())));
            original.call(collector, screenHeight, ticks, mode);
        } finally { collector.defaultParameters(parameters); }
    }
}
