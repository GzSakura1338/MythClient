package me.mioclient;

import me.mioclient.event.Event;
import net.minecraft.entity.player.PlayerEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PlayerEntityEvent.class */
public class PlayerEntityEvent extends Event {
    public final PlayerEntity playerEntity;
    public final int num;
    public final LogoutSpotsHelperMode logoutSpotsHelperMode;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/PlayerEntityEvent$LogoutSpotsHelperMode.class */
    public enum LogoutSpotsHelperMode {
        TOTEM_POP,
        DEATH
    }

    public PlayerEntityEvent(PlayerEntity playerEntity, int i, LogoutSpotsHelperMode logoutSpotsHelperMode) {
        this.playerEntity = playerEntity;
        this.num = i;
        this.logoutSpotsHelperMode = logoutSpotsHelperMode;
    }

    public PlayerEntity getPlayerEntity1890() {
        return this.playerEntity;
    }

    public int get1891() {
        return this.num;
    }

    public LogoutSpotsHelperMode getLogoutSpotsHelperMode1892() {
        return this.logoutSpotsHelperMode;
    }

    public String getName() {
        return this.playerEntity.getGameProfile().getName();
    }
}
