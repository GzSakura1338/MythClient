package today.starain.mixin;

import io.github.humbleui.skija.Font;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.screens.multiplayer.JoinMultiplayerScreen;
import org.lwjgl.glfw.GLFW;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.ui.injection.InjectedJoinScreen;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.context.SkiaContext;
import today.starain.utils.render.skia.font.Fonts;

import java.awt.*;

@Mixin(JoinMultiplayerScreen.class)
public abstract class MixinJoinMultiplayerScreen {

    private static final float BTN_MARGIN_RIGHT = 8f;
    private static final float BTN_MARGIN_TOP = 8f;
    private static final float BTN_PADDING_X = 8f;
    private static final float BTN_PADDING_Y = 5f;
    private static final float BTN_CORNER = 4f;
    private static final float BTN_FONT_SIZE = 9.5f;

    private static final Color BTN_BG_MAIN = new Color(28, 96, 118, 255);
    private static final Color BTN_BG_HOVER = new Color(46, 122, 148, 255);

    private static final String BTN_TEXT =
            new String(new int[]{0x5185, 0x7f6e, 0x8fdb, 0x670d}, 0, 4);

    private static boolean sPrevLmb;

    @Inject(method = "render", at = @At("TAIL"))
    private void starain$renderInjectedJoinOverlay(GuiGraphics guiGraphics,
                                                   int mouseX, int mouseY,
                                                   float partialTick,
                                                   CallbackInfo ci) {
        try {
            SkiaContext.draw((context) -> {
                Skia.save();
                Skia.scale((float) Minecraft.getInstance().getWindow().getGuiScale());

                Font font = Fonts.getNormal(BTN_FONT_SIZE);
                float textW = Skia.getTextWidth(BTN_TEXT, font);
                float bw = textW + BTN_PADDING_X * 2f;
                float bh = BTN_FONT_SIZE + BTN_PADDING_Y * 2f;
                int guiW = Minecraft.getInstance().getWindow().getGuiScaledWidth();
                float bx = guiW - bw - BTN_MARGIN_RIGHT;
                float by = BTN_MARGIN_TOP;

                boolean btnHover = mouseX >= bx && mouseX <= bx + bw &&
                        mouseY >= by && mouseY <= by + bh;
                Color bg = btnHover ? BTN_BG_HOVER : BTN_BG_MAIN;

                Skia.drawRoundedRect(bx, by, bw, bh, BTN_CORNER, bg);
                Skia.drawText(BTN_TEXT, bx + BTN_PADDING_X, by + BTN_PADDING_Y,
                        Color.WHITE, font);

                if (InjectedJoinScreen.isOpen()) {
                    InjectedJoinScreen.render(mouseX, mouseY);
                }

                Skia.restore();

                long window = Minecraft.getInstance().getWindow().getWindow();

                boolean lmb = GLFW.glfwGetMouseButton(window,
                        GLFW.GLFW_MOUSE_BUTTON_LEFT) == GLFW.GLFW_PRESS;
                boolean lmbEdge = lmb && !sPrevLmb;
                sPrevLmb = lmb;

                if (lmbEdge) {
                    if (InjectedJoinScreen.isOpen()) {
                        InjectedJoinScreen.handleClick(mouseX, mouseY);
                    } else if (btnHover) {

                        InjectedJoinScreen.setParent((JoinMultiplayerScreen) (Object) this);
                        InjectedJoinScreen.open();
                    }
                }

            });
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }
}
