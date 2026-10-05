package me.mioclient;

import io.netty.buffer.Unpooled;
import me.mioclient.mixin.ducks.DuckClientWorld;
import net.minecraft.client.network.PendingUpdateManager;
import net.minecraft.entity.Entity;
import net.minecraft.network.ClientConnection;
import net.minecraft.network.PacketByteBuf;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.c2s.play.ClientCommandC2SPacket;
import net.minecraft.network.packet.c2s.play.HandSwingC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerActionC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractBlockC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractItemC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import net.minecraft.network.packet.c2s.play.UpdateSelectedSlotC2SPacket;
import net.minecraft.util.Hand;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoSignSearchHelper4.class */
public class AutoSignSearchHelper4 implements SearchHelper_4 {
    public static void do2556(Hand hand, BlockHitResult blockHitResult) {
        do2571(new PlayerInteractBlockC2SPacket(hand, blockHitResult, get2572()));
    }

    public static void do2557(Hand hand) {
        do2558(hand, minecraftClient.player.getYaw(), minecraftClient.player.getPitch());
    }

    public static void do2558(Hand hand, float f, float f2) {
        do2571(new PlayerInteractItemC2SPacket(hand, get2572(), f, f2));
    }

    public static void do2559(Hand hand) {
        do2571(new HandSwingC2SPacket(hand));
    }

    public static void do2560(float f, float f2, boolean z) {
        do2571(new PlayerMoveC2SPacket.LookAndOnGround(f, f2, z));
    }

    public static void do2561(float[] fArr, boolean z) {
        if (fArr == null || fArr.length < 2) {
            return;
        }
        do2560(fArr[0], fArr[1], z);
    }

    public static void do2562(double d, double d2, double d3, boolean z) {
        do2571(new PlayerMoveC2SPacket.PositionAndOnGround(d, d2, d3, z));
    }

    public static void do2563(double d, double d2, double d3, float f, float f2, boolean z) {
        do2571(new PlayerMoveC2SPacket.Full(d, d2, d3, f, f2, z));
    }

    public static void do2564(int i) {
        PacketByteBuf packetByteBuf = new PacketByteBuf(Unpooled.buffer());
        packetByteBuf.writeVarInt(i);
        packetByteBuf.writeVarInt(1);
        packetByteBuf.writeBoolean(minecraftClient.player.isSneaking());
        do2571(PlayerInteractEntityC2SPacket.CODEC.decode(packetByteBuf));
    }

    public static void do2565(PlayerActionC2SPacket.Action action, BlockPos blockPos, Direction direction) {
        do2571(new PlayerActionC2SPacket(action, blockPos, direction, get2572()));
    }

    public static void do2566(Entity entity, ClientCommandC2SPacket.Mode mode) {
        do2567(entity, mode, 0);
    }

    public static void do2567(Entity entity, ClientCommandC2SPacket.Mode mode, int i) {
        do2571(new ClientCommandC2SPacket(entity, mode, i));
    }

    public static void do948() {
        do2567(minecraftClient.player, ClientCommandC2SPacket.Mode.START_FALL_FLYING, 0);
    }

    public static void do2568() {
        do2571(new PlayerActionC2SPacket(PlayerActionC2SPacket.Action.SWAP_ITEM_WITH_OFFHAND, BlockPos.ORIGIN, Direction.DOWN));
    }

    public static void do2569(int i) {
        do2570(i, false);
    }

    public static void do2570(int i, boolean z) {
        if (z || BaritoneHelper_3.fireworksHelperSearchHelper4.get2632() != i) {
            do2571(new UpdateSelectedSlotC2SPacket(i));
        }
    }

    public static void do2571(Packet<?> packet) {
        minecraftClient.player.networkHandler.sendPacket(packet);
    }

    public static int get2572() {
        PendingUpdateManager incrementSequence = ((DuckClientWorld) (Object) minecraftClient.world).getPendingUpdateManager().incrementSequence();
        if (incrementSequence != null) {
            incrementSequence.close();
        }
        return incrementSequence.getSequence();
    }

    public static void do2573(Packet<?> packet) {
        if (packet == null) {
            return;
        }
        ClientConnection connection = minecraftClient.player.networkHandler.getConnection();
        if (connection.isOpen()) {
            connection.packetsSentCounter++;
            if (connection.channel.eventLoop().inEventLoop()) {
                connection.channel.writeAndFlush(packet);
                return;
            }
            connection.channel.eventLoop().execute(() -> {
                connection.channel.writeAndFlush(packet);
            });
        }
    }
}
