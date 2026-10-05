package me.mioclient;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import me.mioclient.event.DisconnectEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.RemoveEntityEvent;
import me.mioclient.event.RenderEvent;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MainhandHelper_2.class */
public final class MainhandHelper_2 implements SearchHelper_4 {
    public final Map<Integer, PlayerEntitySearchHelper4> map = Collections.synchronizedMap(new HashMap());

    public MainhandHelper_2() {
        baritoneHelper.do1796(this);
    }

    @Listen
    public void onRender(RenderEvent renderEvent) {
        if (is1469()) {
            return;
        }
        for (AbstractClientPlayerEntity abstractClientPlayerEntity : minecraftClient.world.getPlayers()) {
            this.map.putIfAbsent(Integer.valueOf(abstractClientPlayerEntity.getId()), new PlayerEntitySearchHelper4(abstractClientPlayerEntity));
            this.map.get(Integer.valueOf(abstractClientPlayerEntity.getId())).do466();
        }
    }

    @Listen
    public void onRemoveEntity(RemoveEntityEvent removeEntityEvent) {
        synchronized (this.map) {
            this.map.remove(Integer.valueOf(removeEntityEvent.getId()));
        }
    }

    @Listen
    public void onDisconnect(DisconnectEvent disconnectEvent) {
        this.map.clear();
    }

    public synchronized Box getBox1109(PlayerEntity playerEntity, int i) {
        if (!this.map.containsKey(Integer.valueOf(playerEntity.getId()))) {
            return playerEntity.getBoundingBox();
        }
        List<Box> list2712 = this.map.get(Integer.valueOf(playerEntity.getId())).getList2712();
        if (list2712.isEmpty()) {
            return playerEntity.getBoundingBox();
        }
        return list2712.get(Math.min(i, list2712.size() - 1));
    }
}
