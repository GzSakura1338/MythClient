package me.mioclient.module.movement;

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.FreecamHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper4_7;
import me.mioclient.SearchHelper_4;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.mixin.ducks.DuckPlayerMoveC2SPacket;
import me.mioclient.module.Module;
import net.minecraft.block.Blocks;
import net.minecraft.entity.EquipmentSlot;
import net.minecraft.item.Items;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import net.minecraft.util.Hand;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/NoFall.class */
public class NoFall extends Module {
    public Setting<NoFallMode> mode;
    public Setting<Float> groundDistance;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/movement/NoFall$NoFallMode.class */
    public static enum NoFallMode implements EnumSettingHelper {
        noFallMode("Packet") {
            @Override
            public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
                DuckPlayerMoveC2SPacket packet904 = (DuckPlayerMoveC2SPacket)(sendImmediatelyEvent.getPacket904());
                if (packet904 instanceof PlayerMoveC2SPacket) {
                    DuckPlayerMoveC2SPacket duckPlayerMoveC2SPacket = (DuckPlayerMoveC2SPacket)((PlayerMoveC2SPacket) packet904);
                    if (SearchHelper_4.minecraftClient.player.fallDistance <= 3.0f || SearchHelper_4.minecraftClient.player.isFallFlying()) {
                        return;
                    }
                    duckPlayerMoveC2SPacket.setOnGround(true);
                }
            }
        },
        noFallMode2("Anti") {
            @Override
            public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
                if (!(sendImmediatelyEvent.getPacket904() instanceof PlayerMoveC2SPacket) || SearchHelper_4.minecraftClient.player.fallDistance <= 3.0f) {
                    return;
                }
                if (SearchHelper_4.minecraftClient.world.isSpaceEmpty(SearchHelper_4.minecraftClient.player.getBoundingBox().stretch(0.0d, SearchHelper_4.minecraftClient.player.getVelocity().y * 1.1d, 0.0d))) {
                    return;
                }
                SearchHelper_4.minecraftClient.player.setPos(SearchHelper_4.minecraftClient.player.getX(), SearchHelper_4.minecraftClient.player.getY() + 2.0d, SearchHelper_4.minecraftClient.player.getZ());
            }
        },
        noFallMode3("Teleport") {
            @Override
            public void do995(NoFall noFall) {
                if (SearchHelper_4.minecraftClient.player.fallDistance > 3.0f) {
                    HashSet<BlockPos> hashSet2372 = noFall.getHashSet2372(SearchHelper_4.minecraftClient.player.getBoundingBox());
                    hashSet2372.add(BlockPos.ofFloored(SearchHelper_4.minecraftClient.player.getPos()));
                    for (int i = 0; i <= noFall.groundDistance.getValue().floatValue(); i++) {
                        Iterator<BlockPos> it = hashSet2372.iterator();
                        while (it.hasNext()) {
                            if (SearchHelper_4.minecraftClient.world.getBlockState(it.next().offset(Direction.DOWN, i)).getBlock() != Blocks.AIR) {
                                if (SearchHelper4_7.getStashFinderMode2438().is2174()) {
                                    AutoSignSearchHelper4.do2562(0.0d, 64.0d, 0.0d, false);
                                } else {
                                    AutoSignSearchHelper4.do2562(SearchHelper_4.minecraftClient.player.getX(), 0.0d, SearchHelper_4.minecraftClient.player.getZ(), false);
                                }
                                SearchHelper_4.minecraftClient.player.fallDistance = 0.0f;
                            }
                        }
                    }
                }
            }
        },
        noFallMode4("MLG") {
            @Override
            public void do995(NoFall noFall) {
                if (SearchHelper_4.minecraftClient.player.fallDistance > 3.0f) {
                    HashSet<BlockPos> hashSet2372 = noFall.getHashSet2372(SearchHelper_4.minecraftClient.player.getBoundingBox());
                    hashSet2372.add(BlockPos.ofFloored(SearchHelper_4.minecraftClient.player.getPos()));
                    for (int i = 0; i <= noFall.groundDistance.getValue().floatValue(); i++) {
                        Iterator<BlockPos> it = hashSet2372.iterator();
                        while (it.hasNext()) {
                            if (SearchHelper_4.minecraftClient.world.getBlockState(it.next().offset(Direction.DOWN, i)).getBlock() != Blocks.AIR) {
                                boolean z = SearchHelper_4.minecraftClient.player.getOffHandStack().getItem() == Items.WATER_BUCKET;
                                int i2 = FireworksHelper.get447(Items.WATER_BUCKET);
                                if (i2 != -1 || z) {
                                    SearchHelper_4.minecraftClient.player.setPitch(FreecamHelper.num2);
                                    if (!z) {
                                        FireworksHelper.do456(i2);
                                    }
                                    Hand hand = z ? Hand.OFF_HAND : Hand.MAIN_HAND;
                                    SearchHelper_4.minecraftClient.interactionManager.interactItem(SearchHelper_4.minecraftClient.player, hand);
                                    SearchHelper_4.minecraftClient.player.swingHand(hand);
                                }
                            }
                        }
                    }
                }
            }
        },
        noFallMode5("Grim") {
            @Override
            public void do995(NoFall noFall) {
                if (noFall.is1469() || noFall.is2373() || SearchHelper_4.minecraftClient.player.isOnGround() || SearchHelper_4.minecraftClient.player.fallDistance <= 3.0f) {
                    return;
                }
                HashSet<BlockPos> hashSet2372 = noFall.getHashSet2372(SearchHelper_4.minecraftClient.player.getBoundingBox());
                hashSet2372.add(BlockPos.ofFloored(SearchHelper_4.minecraftClient.player.getPos()));
                for (int i = 0; i <= 3; i++) {
                    Iterator<BlockPos> it = hashSet2372.iterator();
                    while (it.hasNext()) {
                        if (SearchHelper_4.minecraftClient.world.getBlockState(it.next().offset(Direction.DOWN, i)).getBlock() != Blocks.AIR) {
                            AutoSignSearchHelper4.do2563(SearchHelper_4.minecraftClient.player.getX(), SearchHelper_4.minecraftClient.player.getY() + 1.0E-9d, SearchHelper_4.minecraftClient.player.getZ(), SearchHelper_4.minecraftClient.player.getYaw(), SearchHelper_4.minecraftClient.player.getPitch(), false);
                        }
                    }
                }
            }
        };

        public final String name;

        NoFallMode(String str2) {
            this.name = str2;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        }

        public void do995(NoFall noFall) {
        }
    }

    public NoFall() {
        super("NoFall", "Prevents falling damage.", Category.MOVEMENT, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        return FontsSearchHelper4.getString1684(this.mode.getValue());
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        this.mode.getValue().do995(this);
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        this.mode.getValue().do30(sendImmediatelyEvent);
    }

    public HashSet<BlockPos> getHashSet2372(Box box) {
        return new HashSet<>(List.of(BlockPos.ofFloored(box.maxX, box.minY, box.maxZ), BlockPos.ofFloored(box.maxX, box.minY, box.minZ), BlockPos.ofFloored(box.minX, box.minY, box.maxZ), BlockPos.ofFloored(box.minX, box.minY, box.minZ)));
    }

    public boolean is2373() {
        return minecraftClient.player.getInventory().getArmorStack(EquipmentSlot.CHEST.getEntitySlotId()).isOf(Items.ELYTRA) && minecraftClient.player.isFallFlying();
    }
}
