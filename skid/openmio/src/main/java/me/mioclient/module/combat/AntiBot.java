package me.mioclient.module.combat;

import me.mioclient.Feature_14;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.module.Module;
import net.minecraft.client.network.PlayerListEntry;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/AntiBot.class */
public class AntiBot extends Module {
    public Setting<Boolean> illegalChars;
    public Setting<Boolean> illegalNames;

    public AntiBot() {
        super("AntiBot", "Removes the bots that will be spawned by some anti cheats.", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        PlayerEntity playerEntity = null;
        synchronized (minecraftClient.world.getPlayers()) {
            for (PlayerEntity playerEntity2 : minecraftClient.world.getPlayers()) {
                if (is867(playerEntity2)) {
                    playerEntity = playerEntity2;
                }
            }
        }
        if (playerEntity != null) {
            playerEntity.setRemoved(Entity.RemovalReason.KILLED);
        }
    }

    public boolean is867(PlayerEntity playerEntity) {
        if (minecraftClient.player == playerEntity || (playerEntity instanceof Feature_14.OtherClientPlayerEntity)) {
            return false;
        }
        String string = playerEntity.getName().getString();
        if ((string.length() < 3 || string.length() > 16) && this.illegalNames.getValue().booleanValue()) {
            return true;
        }
        if (this.illegalChars.getValue().booleanValue()) {
            for (char c : string.toLowerCase().toCharArray()) {
                if (!FontsSearchHelper4.is1685("1234567890_qwertyuiopasdfghjklzxcvbnm", c)) {
                    return true;
                }
            }
        }
        PlayerListEntry playerListEntry = minecraftClient.player.networkHandler.getPlayerListEntry(playerEntity.getGameProfile().getId());
        if (!minecraftClient.player.networkHandler.getPlayerList().contains(playerListEntry) || playerListEntry == null) {
            return true;
        }
        return playerListEntry.getProfile() == null && playerEntity.hasCustomName();
    }
}
