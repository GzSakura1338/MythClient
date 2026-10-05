package me.mioclient;

import java.util.Iterator;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import me.mioclient.PlayerEntityEvent;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.ConnectEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import net.minecraft.client.network.PlayerListEntry;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.network.packet.s2c.play.EntityStatusS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerListS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerRemoveS2CPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/LogoutSpotsHelper.class */
public final class LogoutSpotsHelper implements SearchHelper_4 {
    public final Map<String, Integer> map = new ConcurrentHashMap();
    public final Map<String, Long> map2 = new ConcurrentHashMap();

    public LogoutSpotsHelper() {
        baritoneHelper.do1796(this);
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if (is1469()) {
            return;
        }
        EntityStatusS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof EntityStatusS2CPacket ? (EntityStatusS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet904 instanceof EntityStatusS2CPacket) {
            EntityStatusS2CPacket entityStatusS2CPacket = packet904;
            if (entityStatusS2CPacket.getStatus() == 35) {
                PlayerEntity entity = (entityStatusS2CPacket.getEntity(minecraftClient.world)) instanceof PlayerEntity ? (PlayerEntity) (entityStatusS2CPacket.getEntity(minecraftClient.world)) : null;
                if (entity instanceof PlayerEntity) {
                    PlayerEntity playerEntity = entity;
                    String name = playerEntity.getGameProfile().getName();
                    this.map.compute(name, (str, num) -> {
                        return Integer.valueOf(num == null ? 1 : num.intValue() + 1);
                    });
                    baritoneHelper.getObject1794(new PlayerEntityEvent(playerEntity, this.map.get(name).intValue(), PlayerEntityEvent.LogoutSpotsHelperMode.TOTEM_POP));
                }
            }
        }
        PlayerListS2CPacket packet9042 = (channelRead0Event.getPacket904()) instanceof PlayerListS2CPacket ? (PlayerListS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet9042 instanceof PlayerListS2CPacket) {
            for (PlayerListS2CPacket.Entry entry : packet9042.getPlayerAdditionEntries()) {
                this.map2.remove(entry.profile().getName());
            }
        }
        PlayerRemoveS2CPacket packet9043 = (channelRead0Event.getPacket904()) instanceof PlayerRemoveS2CPacket ? (PlayerRemoveS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet9043 instanceof PlayerRemoveS2CPacket) {
            Iterator it = packet9043.profileIds().iterator();
            while (it.hasNext()) {
                PlayerListEntry playerListEntry = minecraftClient.player.networkHandler.getPlayerListEntry((UUID) it.next());
                if (playerListEntry != null) {
                    this.map2.put(playerListEntry.getProfile().getName(), Long.valueOf(System.currentTimeMillis()));
                }
            }
        }
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        for (PlayerEntity playerEntity : minecraftClient.world.getPlayers()) {
            if (playerEntity != null && SearchHelper_3.get644((Entity) playerEntity) <= 0.0f) {
                String string = playerEntity.getName().getString();
                if (this.map.containsKey(string)) {
                    int intValue = this.map.get(string).intValue();
                    this.map.remove(string);
                    baritoneHelper.getObject1794(new PlayerEntityEvent(playerEntity, intValue, PlayerEntityEvent.LogoutSpotsHelperMode.DEATH));
                }
            }
        }
        String str = null;
        for (Map.Entry<String, Long> entry : this.map2.entrySet()) {
            if (entry.getValue().longValue() + 120000 < System.currentTimeMillis()) {
                str = entry.getKey();
                this.map.remove(entry.getKey());
            }
        }
        if (str != null) {
            this.map2.remove(str);
        }
    }

    @Listen
    public void onConnect(ConnectEvent connectEvent) {
        if (BaritoneHelper_3.holeSnapSearchHelper4_4.getServerInfo2622() == null || connectEvent.getString518().equals(BaritoneHelper_3.holeSnapSearchHelper4_4.getServerInfo2622().address)) {
            this.map.clear();
        }
    }

    public int get895(PlayerEntity playerEntity) {
        return get896(playerEntity.getGameProfile().getName());
    }

    public int get896(String str) {
        return this.map.getOrDefault(str, 0).intValue();
    }
}
