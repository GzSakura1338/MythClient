package dev.myth.UI.hud;

import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.PacketEvent;
import java.util.ArrayDeque;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import net.minecraft.client.Minecraft;
import net.minecraft.client.multiplayer.ClientLevel;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.BundlePacket;
import net.minecraft.network.protocol.game.ClientboundEntityEventPacket;
import net.minecraft.network.protocol.game.ClientboundSetTimePacket;
import net.minecraft.network.protocol.game.ClientboundTabListPacket;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.world.level.chunk.LevelChunk;

/** HUD-only parts of Mio speed, latency, loaded block-entity and pop trackers. */
public final class HudData {
    private ClientLevel level;
    private final ArrayDeque<Double> speedSamples = new ArrayDeque<>();
    private final ArrayDeque<Float> tpsSamples = new ArrayDeque<>();
    private final Map<UUID, Integer> pops = new HashMap<>();
    private List<BlockEntity> blockEntities = List.of();
    private volatile long lastPacket = System.currentTimeMillis(), lastTime;
    private volatile float tps = 20;
    private volatile int footerPing;
    private double speed;
    private long lastSpeedRefresh;
    private long tick;
    @EventTarget private void packet(PacketEvent event) {
        if (event.getType() != EventType.INCOMING) return;
        accept(event.getPacket());
    }
    private void accept(Packet<?> packet) {
        long now = System.currentTimeMillis(); lastPacket = now;
        if (packet instanceof BundlePacket<?> bundle) { for (Packet<?> entry : bundle.subPackets()) accept(entry); return; }
        if (packet instanceof ClientboundSetTimePacket) synchronized (tpsSamples) {
            if (lastTime != 0) {
                tpsSamples.add(Math.clamp(20000f / Math.max(1, now - lastTime), 0, 20));
                while (tpsSamples.size() > 20) tpsSamples.removeFirst();
                tps = (float) tpsSamples.stream().mapToDouble(Float::doubleValue).average().orElse(20);
            }
            lastTime = now;
        }
        if (packet instanceof ClientboundTabListPacket tab) {
            var match = java.util.regex.Pattern.compile("\\d+ ping").matcher(tab.footer().getString());
            if (match.find()) try { footerPing = Integer.parseInt(match.group().replace(" ping", "")); } catch (NumberFormatException ignored) {}
        }
        if (packet instanceof ClientboundEntityEventPacket status && status.getEventId() == 35) {
            Minecraft.getInstance().execute(() -> {
                var client = Minecraft.getInstance();
                if (client.level == null) return;
                Entity entity = status.getEntity(client.level);
                if (entity != null) pops.merge(entity.getUUID(), 1, Integer::sum);
            });
        }
    }
    void tick() {
        Minecraft client = Minecraft.getInstance();
        if (level != client.level) {
            level = client.level; speedSamples.clear(); pops.clear(); blockEntities = List.of(); speed = 0;
            synchronized (tpsSamples) { tpsSamples.clear(); lastTime = 0; tps = 20; }
            footerPing = 0; lastPacket = System.currentTimeMillis();
        }
        if (client.player == null || level == null) return;
        Entity entity = client.player.isPassenger() ? client.player.getVehicle() : client.player;
        double value = Math.hypot(entity.getX() - entity.xo, entity.getZ() - entity.zo) * 20 * 3.6;
        if (value > 0 || client.player.tickCount % 4 == 0) speedSamples.add(value); else speedSamples.pollFirst();
        while (speedSamples.size() > 9) speedSamples.removeFirst();
        double average = speedSamples.stream().mapToDouble(Double::doubleValue).average().orElse(0);
        long now = System.currentTimeMillis();
        if (average < 22 || now - lastSpeedRefresh >= 50) { speed = average; lastSpeedRefresh = now; }
        pops.keySet().removeIf(id -> level.players().stream().noneMatch(p -> p.getUUID().equals(id)));
        for (var player : level.players()) if (!player.isAlive()) pops.remove(player.getUUID());
        if (++tick % 20 == 0) {
            var collected = new java.util.ArrayList<BlockEntity>();
            var pos = client.player.chunkPosition(); int radius = Math.min(32, client.options.renderDistance().get());
            for (int x = pos.x() - radius; x <= pos.x() + radius; x++) for (int z = pos.z() - radius; z <= pos.z() + radius; z++) {
                LevelChunk chunk = level.getChunkSource().getChunk(x, z, false);
                if (chunk != null) collected.addAll(chunk.getBlockEntities().values());
            }
            blockEntities = List.copyOf(collected);
        }
    }
    public double speedKmh() { return speed; }
    public float tps() { return Minecraft.getInstance().isLocalServer() ? 20 : tps; }
    public int ping() {
        var client = Minecraft.getInstance();
        if (client.player == null || client.getConnection() == null) return 0;
        var info = client.getConnection().getPlayerInfo(client.player.getUUID());
        int ping = info == null ? 0 : info.getLatency(); return ping == 0 ? footerPing : ping;
    }
    public long silenceMillis() { return Math.max(0, System.currentTimeMillis() - lastPacket); }
    public int pops(UUID id) { return pops.getOrDefault(id, 0); }
    public List<BlockEntity> blockEntities() { return blockEntities; }
}
