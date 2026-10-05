package me.mioclient;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.ExplosionVelocityEvent;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.client.AntiCheat;
import me.mioclient.module.movement.Velocity;
import me.mioclient.module.player.SpeedMine;
import net.minecraft.network.packet.c2s.play.PlayerActionC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import net.minecraft.network.packet.s2c.play.EntityVelocityUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerPositionLookS2CPacket;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/VelocityHelper_3.class */
public class VelocityHelper_3 extends VelocityHelper {
    public static final SpeedMine speedMine = (SpeedMine) BaritoneHelper_3.baritoneHelper_4.getModule117(SpeedMine.class);
    public static final AntiCheat antiCheat = (AntiCheat) BaritoneHelper_3.baritoneHelper_4.getModule117(AntiCheat.class);
    public final Stopwatch stopwatch;
    public final List<Long> list;
    public boolean flag;
    public float val;
    public float val2;

    public VelocityHelper_3(Velocity velocity) {
        super(velocity);
        this.stopwatch = new Stopwatch();
        this.list = Collections.synchronizedList(new ArrayList());
    }

    @Override // me.mioclient.VelocityHelper
    public void do29(ChannelRead0Event channelRead0Event) {
        EntityVelocityUpdateS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof EntityVelocityUpdateS2CPacket ? (EntityVelocityUpdateS2CPacket) (channelRead0Event.getPacket904()) : null;
        if ((packet904 instanceof EntityVelocityUpdateS2CPacket) && packet904.getEntityId() == minecraftClient.player.getId() && !is715()) {
            channelRead0Event.do1162();
        }
        if (channelRead0Event.getPacket904() instanceof PlayerPositionLookS2CPacket) {
            this.list.add(Long.valueOf(System.currentTimeMillis()));
        }
    }

    @Override // me.mioclient.VelocityHelper
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        PlayerMoveC2SPacket packet904 = (sendImmediatelyEvent.getPacket904()) instanceof PlayerMoveC2SPacket ? (PlayerMoveC2SPacket) (sendImmediatelyEvent.getPacket904()) : null;
        if (packet904 instanceof PlayerMoveC2SPacket) {
            PlayerMoveC2SPacket playerMoveC2SPacket = packet904;
            if (playerMoveC2SPacket.changesLook()) {
                this.val = playerMoveC2SPacket.getYaw(0.0f);
                this.val2 = playerMoveC2SPacket.getPitch(0.0f);
            }
        }
    }

    @Override // me.mioclient.VelocityHelper
    public void do598(ExplosionVelocityEvent explosionVelocityEvent) {
        if (((explosionVelocityEvent.get767() == 0.0f && explosionVelocityEvent.get769() == 0.0f && explosionVelocityEvent.get771() == 0.0f) ? false : true) && this.velocity.explosions.getValue().booleanValue() && !is715()) {
            explosionVelocityEvent.do768(0.0f);
            explosionVelocityEvent.do770(0.0f);
            explosionVelocityEvent.do772(0.0f);
        }
    }

    @Override // me.mioclient.VelocityHelper
    public void do711() {
        this.flag = BlockPos.stream(minecraftClient.player.getBoundingBox()).anyMatch(blockPos -> {
            return !minecraftClient.world.isAir(blockPos);
        });
        if (minecraftClient.player.age <= 20 || is715()) {
            return;
        }
        this.list.removeIf(l -> {
            return System.currentTimeMillis() > l.longValue() + 1000;
        });
        if (this.list.size() <= 2) {
            do712();
        } else {
            this.stopwatch.reset();
            this.list.clear();
        }
    }

    public void do712() {
        BlockPos blockPos713 = getBlockPos713();
        if (blockPos713 != null) {
            AutoSignSearchHelper4.do2571(new PlayerMoveC2SPacket.Full(minecraftClient.player.getX(), minecraftClient.player.getY(), minecraftClient.player.getZ(), this.val, this.val2, minecraftClient.player.isOnGround()));
            AutoSignSearchHelper4.do2571(new PlayerActionC2SPacket(PlayerActionC2SPacket.Action.STOP_DESTROY_BLOCK, blockPos713, Direction.DOWN));
        }
    }

    public BlockPos getBlockPos713() {
        return (BlockPos) BlockPos.stream(minecraftClient.player.getBoundingBox()).map((v0) -> {
            return v0.toImmutable();
        }).filter(SearchHelper4_7::is2446).max(Comparator.comparing(this::get714)).orElse(null);
    }

    public float get714(BlockPos blockPos) {
        return (float) minecraftClient.player.getBoundingBox().intersection(new Box(blockPos)).getAverageSideLength();
    }

    public boolean is715() {
        return minecraftClient.player.isFallFlying() || HoleSnapSearchHelper4.is2005(minecraftClient.player) || (getBlockPos713() == null && !this.flag) || !this.stopwatch.is419(200L);
    }
}
