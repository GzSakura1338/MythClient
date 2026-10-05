package me.mioclient.module.misc;

import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper4_7;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.DisconnectEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.module.Module;
import net.minecraft.network.packet.s2c.play.DeathMessageS2CPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/CoordLogger.class */
public class CoordLogger extends Module {
    public Setting<Boolean> log;
    public Setting<Boolean> death;
    public String text;

    public CoordLogger() {
        super("CoordLogger", "Copies your coordinates whenever you log/die.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        this.text = "X: %d, Y: %d, Z: %d in The %s".formatted(Integer.valueOf(minecraftClient.player.getBlockX()), Integer.valueOf(minecraftClient.player.getBlockY()), Integer.valueOf(minecraftClient.player.getBlockZ()), SearchHelper4_7.getStashFinderMode2438().getString2175());
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if ((channelRead0Event.getPacket904() instanceof DeathMessageS2CPacket) && this.death.getValue().booleanValue()) {
            do906();
        }
    }

    @Listen
    public void onDisconnect(DisconnectEvent disconnectEvent) {
        if (minecraftClient.player == null || !this.log.getValue().booleanValue()) {
            return;
        }
        do906();
    }

    public void do906() {
        minecraftClient.keyboard.setClipboard(this.text);
    }
}
