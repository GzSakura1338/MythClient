package me.mioclient;

import net.minecraft.client.MinecraftClient;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.ingame.ShulkerBoxScreen;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.ShulkerBoxScreenHandler;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;
import org.jetbrains.annotations.Nullable;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/TooltipsShulkerBoxScreen.class */
public class TooltipsShulkerBoxScreen extends ShulkerBoxScreen implements SearchHelper_4 {
    public final Screen screen;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/TooltipsShulkerBoxScreen$DrawContext.class */
    private static class DrawContext extends net.minecraft.client.gui.DrawContext {
        public DrawContext(net.minecraft.client.gui.DrawContext drawContext) {
            super(MinecraftClient.getInstance(), drawContext.getVertexConsumers());
        }

        public void drawItemInSlot(TextRenderer textRenderer, ItemStack itemStack, int i, int i2, @Nullable String str) {
            if (itemStack.getCount() == 69) {
                str = new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("\u00010");
            }
            super.drawItemInSlot(textRenderer, itemStack, i, i2, str);
        }
    }

    public TooltipsShulkerBoxScreen(ShulkerBoxScreenHandler shulkerBoxScreenHandler, PlayerInventory playerInventory, Text text, Screen screen) {
        super(shulkerBoxScreenHandler, playerInventory, text);
        this.screen = screen;
    }

    public void render(net.minecraft.client.gui.DrawContext drawContext, int i, int i2, float f) {
        super.render(new DrawContext(drawContext), i, i2, f);
    }

    public boolean mouseClicked(double d, double d2, int i) {
        return false;
    }

    public boolean keyPressed(int i, int i2, int i3) {
        if (i != 256) {
            return super.keyPressed(i, i2, i3);
        }
        minecraftClient.setScreen(this.screen);
        return true;
    }
}
