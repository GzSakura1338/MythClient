package me.mioclient.feature;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import me.mioclient.SearchHelper_4;
import net.minecraft.client.gui.screen.ConfirmScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/ConfirmDisconnect.class */
public class ConfirmDisconnect extends ConfirmScreen {

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/feature/ConfirmDisconnect$BooleanConsumer.class */
    private static final class BooleanConsumer implements it.unimi.dsi.fastutil.booleans.BooleanConsumer, SearchHelper_4 {
        public final Runnable runnable;

        public BooleanConsumer(Runnable runnable) {
            this.runnable = runnable;
        }

        public void accept(boolean z) {
            minecraftClient.setScreen((Screen) null);
            if (z) {
                this.runnable.run();
            }
        }




        public Runnable getRunnable2145() {
            return this.runnable;
        }
    }

    public ConfirmDisconnect(Runnable runnable) {
        super(new BooleanConsumer(runnable), Text.literal("Confirm Disconnect"), Text.literal("Are you sure that you want to disconnect?"));
    }
}
