package me.mioclient.module.combat;

import java.util.HashMap;
import java.util.Map;
import me.mioclient.FireworksHelper;
import me.mioclient.LegacyCrystalSearchHelper4;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.api.Category;
import me.mioclient.mixin.ducks.DuckMinecraftClient;
import me.mioclient.api.Setting;
import me.mioclient.event.InteractBlockEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickPostEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.entity.Entity;
import net.minecraft.entity.decoration.EndCrystalEntity;
import net.minecraft.item.Items;
import net.minecraft.item.SwordItem;
import net.minecraft.network.packet.c2s.play.PlayerInteractBlockC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.util.hit.EntityHitResult;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/LegacyCrystal.class */
public class LegacyCrystal extends Module {
    public Setting<Integer> setting;
    public Setting<Boolean> setting2;
    public final Map<BlockPos, Long> map;
    public final Stopwatch stopwatch;
    public long num;

    public LegacyCrystal() {
        super("LegacyCrystal", "Legit auto crystal (crystal aura).", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.map = new HashMap();
        this.stopwatch = new Stopwatch();
        this.num = 150L;
    }

    @Listen
    public void onTickPost(TickPostEvent tickPostEvent) {
        synchronized (this.map) {
            this.map.entrySet().removeIf(entry -> {
                return ((Long) entry.getValue()).longValue() + 15000 < System.currentTimeMillis();
            });
        }
        if (minecraftClient.crosshairTarget instanceof EntityHitResult) {
            Entity entity = ((EntityHitResult) minecraftClient.crosshairTarget).getEntity();
            if (this.stopwatch.is419(this.num) && (entity instanceof EndCrystalEntity)) {
                if (this.map.containsKey(entity.getBlockPos().down())) {
                    ((DuckMinecraftClient)(Object) minecraftClient).attack();
                    this.num = (long) (Math.random() * this.setting.getValue().intValue());
                    this.stopwatch.reset();
                }
            }
        }
    }

    @Listen
    public void onInteractBlock(InteractBlockEvent interactBlockEvent) {
        if (minecraftClient.world.getBlockState(interactBlockEvent.getBlockHitResult2585().getBlockPos().up(2)).isAir()) {
            boolean is3043 = PhaseESPSearchHelper4_2.is3043(interactBlockEvent.getBlockHitResult2585().getBlockPos(), false, false, true, false, false, false);
            if (minecraftClient.player.getMainHandStack().getItem() instanceof SwordItem) {
                FireworksHelper.do456(FireworksHelper.get447(is3043 ? Items.END_CRYSTAL : Items.OBSIDIAN));
                return;
            }
            if (minecraftClient.player.getMainHandStack().isOf(Items.OBSIDIAN) && is3043) {
                FireworksHelper.do438(FireworksHelper.get447(Items.END_CRYSTAL));
            }
        }
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        PlayerInteractBlockC2SPacket packet904 = (sendImmediatelyEvent.getPacket904()) instanceof PlayerInteractBlockC2SPacket ? (PlayerInteractBlockC2SPacket) (sendImmediatelyEvent.getPacket904()) : null;
        if (packet904 instanceof PlayerInteractBlockC2SPacket) {
            PlayerInteractBlockC2SPacket playerInteractBlockC2SPacket = packet904;
            if (minecraftClient.player.getStackInHand(playerInteractBlockC2SPacket.getHand()).isOf(Items.END_CRYSTAL)) {
                this.map.compute(playerInteractBlockC2SPacket.getBlockHitResult().getBlockPos(), (blockPos, l) -> {
                    return Long.valueOf(System.currentTimeMillis());
                });
            }
        }
        PlayerInteractEntityC2SPacket packet9042 = (sendImmediatelyEvent.getPacket904()) instanceof PlayerInteractEntityC2SPacket ? (PlayerInteractEntityC2SPacket) (sendImmediatelyEvent.getPacket904()) : null;
        if (packet9042 instanceof PlayerInteractEntityC2SPacket) {
            PlayerInteractEntityC2SPacket playerInteractEntityC2SPacket = packet9042;
            if (this.setting2.getValue().booleanValue() && LegacyCrystalSearchHelper4.getLegacyCrystalMode2611(playerInteractEntityC2SPacket) == LegacyCrystalSearchHelper4.LegacyCrystalMode.ATTACK && (LegacyCrystalSearchHelper4.getEntity2610(playerInteractEntityC2SPacket) instanceof EndCrystalEntity)) {
                LegacyCrystalSearchHelper4.getEntity2610(playerInteractEntityC2SPacket).setRemoved(Entity.RemovalReason.KILLED);
            }
        }
    }
}
