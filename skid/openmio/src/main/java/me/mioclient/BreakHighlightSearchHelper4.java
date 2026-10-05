package me.mioclient;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.IllegalConstructorCall;
import me.mioclient.module.player.SpeedMine;
import net.minecraft.enchantment.Enchantments;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.network.packet.s2c.play.BlockBreakingProgressS2CPacket;
import net.minecraft.network.packet.s2c.play.BlockUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDeltaUpdateS2CPacket;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BreakHighlightSearchHelper4.class */
public final class BreakHighlightSearchHelper4 implements SearchHelper_4 {
    public static final SpeedMine speedmine = (SpeedMine) BaritoneHelper_3.baritoneHelper_4.getModule117(SpeedMine.class);
    public static ItemStack itemStack;
    public final List<EntityIdHelper> list = Collections.synchronizedList(new ArrayList());

    public BreakHighlightSearchHelper4() {
        baritoneHelper.do1796(this);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        float f;
        this.list.removeIf((v0) -> {
            return v0.is2817();
        });
        for (EntityIdHelper entityIdHelper : this.list) {
            float f2 = ((float) SearchHelper4_7.get2436(getItemStack1518(), minecraftClient.world.getBlockState(entityIdHelper.getBlockPos386()), true)) * BaritoneHelper_3.holeSnapSearchHelper4_4.get2621();
            if (speedmine.isToggled()) {
                f2 /= speedmine.damage.getValue().floatValue();
            }
            if (SearchHelper4_7.is2435(entityIdHelper.getBlockPos386())) {
                f = f2 * Math.max(entityIdHelper.get2093(), 1);
                entityIdHelper.do2815(0);
            } else {
                f = 0.0f;
                entityIdHelper.do2815(entityIdHelper.get2093() + 1);
                entityIdHelper.do2816();
            }
            entityIdHelper.do2144(entityIdHelper.get2142() + f);
        }
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        BlockBreakingProgressS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof BlockBreakingProgressS2CPacket ? (BlockBreakingProgressS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet904 instanceof BlockBreakingProgressS2CPacket) {
            BlockBreakingProgressS2CPacket blockBreakingProgressS2CPacket = packet904;
            minecraftClient.execute(() -> {
                if (blockBreakingProgressS2CPacket.getProgress() < 0 || blockBreakingProgressS2CPacket.getProgress() > 10) {
                    return;
                }
                do1515(blockBreakingProgressS2CPacket.getPos(), blockBreakingProgressS2CPacket.getEntityId());
            });
        }
        BlockUpdateS2CPacket packet9042 = (channelRead0Event.getPacket904()) instanceof BlockUpdateS2CPacket ? (BlockUpdateS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet9042 instanceof BlockUpdateS2CPacket) {
            BlockUpdateS2CPacket blockUpdateS2CPacket = packet9042;
            if (blockUpdateS2CPacket.getState().isAir()) {
                do1514(blockUpdateS2CPacket.getPos());
            }
        }
        ChunkDeltaUpdateS2CPacket packet9043 = (channelRead0Event.getPacket904()) instanceof ChunkDeltaUpdateS2CPacket ? (ChunkDeltaUpdateS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet9043 instanceof ChunkDeltaUpdateS2CPacket) {
            packet9043.visitUpdates((blockPos, blockState) -> {
                if (blockState.isAir()) {
                    do1514(blockPos);
                }
            });
        }
    }

    public void do1514(BlockPos blockPos) {
        if (speedmine.rebreak2.getValue() == SpeedMineMode_2.INSTANT) {
            return;
        }
        minecraftClient.execute(() -> {
            for (EntityIdHelper entityIdHelper : this.list) {
                if (blockPos.equals(entityIdHelper.getBlockPos386())) {
                    entityIdHelper.do2144(0.0f);
                    entityIdHelper.do2815(0);
                }
            }
        });
    }

    public void do1515(BlockPos blockPos, int i) {
        boolean z = false;
        Iterator<EntityIdHelper> it = this.list.iterator();
        while (true) {
            if (!it.hasNext()) {
                break;
            }
            EntityIdHelper next = it.next();
            if (next.getEntityId() == i && next.getBlockPos386().equals(blockPos)) {
                z = true;
                break;
            }
        }
        if (z) {
            return;
        }
        this.list.removeIf(entityIdHelper -> {
            return entityIdHelper.getBlockPos386().equals(blockPos) || entityIdHelper.getEntityId() == i;
        });
        if (SearchHelper4_7.is2435(blockPos)) {
            this.list.add(new EntityIdHelper(i, blockPos));
        }
    }

    public float get1516(BlockPos blockPos) {
        if (!SearchHelper4_7.is2435(blockPos)) {
            return 0.0f;
        }
        return ((Float) this.list.stream().filter(entityIdHelper -> {
            return entityIdHelper.getBlockPos386().equals(blockPos);
        }).map((v0) -> {
            return v0.get2142();
        }).findAny().orElse(Float.valueOf(0.0f))).floatValue();
    }

    public List<EntityIdHelper> getList1517() {
        return this.list;
    }

    public static ItemStack getItemStack1518() {
        if (itemStack == null) {
            itemStack = new ItemStack(Items.NETHERITE_PICKAXE);
            itemStack.addEnchantment(IllegalConstructorCall.getRegistryEntry1420(Enchantments.EFFICIENCY), 5);
        }
        return itemStack;
    }
}
