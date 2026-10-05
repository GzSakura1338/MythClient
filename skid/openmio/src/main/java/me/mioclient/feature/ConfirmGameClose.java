package me.mioclient.feature;

import me.mioclient.SearchHelper_4;
import me.mioclient.mixin.ducks.DuckMinecraftClient;
import net.minecraft.client.gui.screen.ConfirmScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.text.Text;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/ConfirmGameClose.class */
public class ConfirmGameClose extends ConfirmScreen {

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/feature/ConfirmGameClose$BooleanConsumer.class */
    private static final class BooleanConsumer implements it.unimi.dsi.fastutil.booleans.BooleanConsumer, SearchHelper_4 {
        public void accept(boolean z) {
            if (!((DuckMinecraftClient) minecraftClient).mio$isDisconnecting()) {
                minecraftClient.setScreen((Screen) null);
            }
            if (z) {
                minecraftClient.stop();
            } else {
                GLFW.glfwSetWindowShouldClose(minecraftClient.getWindow().getHandle(), false);
            }
        }
    }

    public ConfirmGameClose() {
        super(new BooleanConsumer(), Text.literal("Confirm Game Close"), Text.literal("Are you sure that you want to close minecraft?"));
    }
}
