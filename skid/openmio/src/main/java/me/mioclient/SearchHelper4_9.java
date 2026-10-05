package me.mioclient;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.function.BiConsumer;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.event.LoadChunkFromPacketEvent;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.network.packet.s2c.play.BlockUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDeltaUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ExplosionS2CPacket;
import net.minecraft.network.packet.s2c.play.UnloadChunkS2CPacket;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.world.chunk.WorldChunk;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_9.class */
public class SearchHelper4_9 implements SearchHelper_4 {
    public final int num;
    public final BiConsumer<ChunkPos, WorldChunk> biConsumer;
    public final BiConsumer<ChunkPos, WorldChunk> biConsumer2;
    public final BiConsumer<BlockPos, BlockState> biConsumer3;
    public ExecutorService executorService;

    public SearchHelper4_9(int i, BiConsumer<ChunkPos, WorldChunk> biConsumer, BiConsumer<ChunkPos, WorldChunk> biConsumer2, BiConsumer<BlockPos, BlockState> biConsumer3) {
        this.num = i;
        this.biConsumer = biConsumer;
        this.biConsumer2 = biConsumer2;
        this.biConsumer3 = biConsumer3;
    }

    public void do1640() {
        this.executorService = Executors.newFixedThreadPool(Math.max(this.num, 1));
        baritoneHelper.do1796(this);
    }

    public void do2687() {
        baritoneHelper.do1802(this);
        this.executorService.shutdownNow();
        this.executorService = null;
    }

    public void do2688() {
        this.executorService.shutdownNow();
        this.executorService = Executors.newFixedThreadPool(Math.max(this.num, 1));
    }

    public void do2689() {
        if (this.biConsumer != null) {
            for (WorldChunk worldChunk : SearchHelper4_7.getList2426()) {
                do2691(() -> {
                    this.biConsumer.accept(worldChunk.getPos(), worldChunk);
                });
            }
        }
    }

    @Listen
    public void onChannelRead0(ChannelRead0Event channelRead0Event) {
        if (minecraftClient.world == null) {
            return;
        }
        if (this.biConsumer3 != null) {
            BlockUpdateS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof BlockUpdateS2CPacket ? (BlockUpdateS2CPacket) (channelRead0Event.getPacket904()) : null;
            if (packet904 instanceof BlockUpdateS2CPacket) {
                BlockUpdateS2CPacket blockUpdateS2CPacket = packet904;
                do2691(() -> {
                    this.biConsumer3.accept(blockUpdateS2CPacket.getPos(), blockUpdateS2CPacket.getState());
                });
                return;
            }
        }
        if (this.biConsumer3 != null) {
            ExplosionS2CPacket packet9042 = (channelRead0Event.getPacket904()) instanceof ExplosionS2CPacket ? (ExplosionS2CPacket) (channelRead0Event.getPacket904()) : null;
            if (packet9042 instanceof ExplosionS2CPacket) {
                for (BlockPos blockPos : packet9042.getAffectedBlocks()) {
                    do2691(() -> {
                        this.biConsumer3.accept(blockPos, Blocks.AIR.getDefaultState());
                    });
                }
                return;
            }
        }
        if (this.biConsumer3 != null) {
            ChunkDeltaUpdateS2CPacket packet9043 = (channelRead0Event.getPacket904()) instanceof ChunkDeltaUpdateS2CPacket ? (ChunkDeltaUpdateS2CPacket) (channelRead0Event.getPacket904()) : null;
            if (packet9043 instanceof ChunkDeltaUpdateS2CPacket) {
                packet9043.visitUpdates((blockPos2, blockState) -> {
                    BlockPos immutable = blockPos2.toImmutable();
                    do2691(() -> {
                        this.biConsumer3.accept(immutable, blockState);
                    });
                });
                return;
            }
        }
        if (this.biConsumer2 != null) {
            UnloadChunkS2CPacket packet9044 = (channelRead0Event.getPacket904()) instanceof UnloadChunkS2CPacket ? (UnloadChunkS2CPacket) (channelRead0Event.getPacket904()) : null;
            if (packet9044 instanceof UnloadChunkS2CPacket) {
                UnloadChunkS2CPacket unloadChunkS2CPacket = packet9044;
                ChunkPos chunkPos = new ChunkPos(unloadChunkS2CPacket.pos().x, unloadChunkS2CPacket.pos().z);
                WorldChunk chunk = minecraftClient.world.getChunk(chunkPos.x, chunkPos.z);
                do2691(() -> {
                    this.biConsumer2.accept(chunkPos, chunk);
                });
            }
        }
    }

    @Listen
    public void onLoadChunkFromPacket(LoadChunkFromPacketEvent loadChunkFromPacketEvent) {
        if (this.biConsumer == null) {
            return;
        }
        do2691(() -> {
            this.biConsumer.accept(loadChunkFromPacketEvent.getWorldChunk2555().getPos(), loadChunkFromPacketEvent.getWorldChunk2555());
        });
    }

    public ExecutorService getExecutorService2690() {
        return this.executorService;
    }

    public void do2691(java.lang.Runnable runnable) {
        if (this.num == 0) {
            minecraftClient.executeSync(runnable);
        } else {
            this.executorService.execute(runnable);
        }
    }
}
