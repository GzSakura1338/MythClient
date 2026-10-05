package me.mioclient.module.misc;

import me.mioclient.api.Category;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.client.gui.screen.DeathScreen;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/AutoRespawn.class */
public class AutoRespawn extends Module {
    public final Stopwatch stopwatch;

    public AutoRespawn() {
        super("AutoRespawn", "Respawns automatically.", Category.MISC, new String[0]);
        this.stopwatch = new Stopwatch();
        setDrawn(false);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if ((minecraftClient.currentScreen instanceof DeathScreen) && this.stopwatch.is419(150L)) {
            minecraftClient.player.requestRespawn();
            this.stopwatch.reset();
        }
    }
}
