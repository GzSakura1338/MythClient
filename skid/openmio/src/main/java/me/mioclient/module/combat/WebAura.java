package me.mioclient.module.combat;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Objects;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FireworksHelper;
import me.mioclient.FreecamHelper;
import me.mioclient.HoleSnapSearchHelper4;
import me.mioclient.MainhandHelper_2;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper;
import me.mioclient.SearchHelper4_7;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Delay;
import me.mioclient.module.player.SpeedMine;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Items;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3i;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/WebAura.class */
public class WebAura extends Delay {
    public static final SpeedMine speedMine = (SpeedMine) BaritoneHelper_3.baritoneHelper_4.getModule117(SpeedMine.class);
    public Setting<Float> range;
    public Setting<Boolean> multiTask;
    public Setting<Boolean> ignoreNakeds;
    public Setting<Boolean> onlyFish;
    public Setting<Boolean> selfSafety;
    public Setting<Boolean> extrapolation;
    public Setting<Integer> ticks;
    public Setting<Boolean> positions;
    public Setting<Boolean> head;
    public Setting<Boolean> legs;
    public PlayerEntity playerEntity;

    public WebAura() {
        super("WebAura", "Traps your enemies with cobwebs.", Category.COMBAT, "autoweb");
        PhaseESPHelper.do1351(this);
        this.setting9.do2333(false);
        this.setting7.do2333(false);
        unregister((Setting<?>) this.setting8);
        unregister((Setting<?>) this.setting9);
        unregister((Setting<?>) this.setting10);
        unregister((Setting<?>) this.setting11);
        unregister((Setting<?>) this.setting7);
    }

    @Override // me.mioclient.module.Delay
    public boolean is1330() {
        return false;
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        if (this.playerEntity != null) {
            return this.playerEntity.getGameProfile().getName();
        }
        return null;
    }

    @Override // me.mioclient.module.Delay
    public List<BlockPos> getList876() {
        this.playerEntity = getPlayerEntity886();
        if (this.playerEntity == null || (!this.multiTask.getValue().booleanValue() && minecraftClient.player.isUsingItem())) {
            return Collections.emptyList();
        }
        ArrayList<net.minecraft.util.math.BlockPos> arrayList = new ArrayList<>();
        Box boundingBox = minecraftClient.player.getBoundingBox();
        if (this.extrapolation.getValue().booleanValue()) {
            MainhandHelper_2 mainhandHelper_2 = BaritoneHelper_3.mainhandHelper_2;
            boundingBox = mainhandHelper_2.getBox1109(this.playerEntity, SearchHelper.get231(this.playerEntity, this.ticks));
        }
        BlockPos ofFloored = BlockPos.ofFloored(SearchHelper.getVec3d222(boundingBox));
        if (this.legs.getValue().booleanValue() && boundingBox.minY - ofFloored.getY() <= FreecamHelper.val2) {
            arrayList.add(ofFloored.down());
            arrayList.add(ofFloored);
        }
        if (this.head.getValue().booleanValue()) {
            arrayList.add(ofFloored.up());
        }
        arrayList.removeIf(blockPos -> {
            return (minecraftClient.world.getBlockState(blockPos).isReplaceable() && minecraftClient.world.getFluidState(blockPos).isEmpty() && is2731(blockPos) && (SearchHelper4_7.is2431((Vec3i) blockPos) || !this.setting5.getValue().booleanValue())) ? false : true;
        });
        return arrayList;
    }

    @Override // me.mioclient.module.Delay
    public int get499() {
        return FireworksHelper.get447(Items.COBWEB);
    }

    @Override // me.mioclient.module.Delay
    public void do1321(BlockPos blockPos, boolean z) {
        if (blockPos.equals(speedMine.getBlockPos1054())) {
            speedMine.do1048();
        }
    }

    public boolean is2731(BlockPos blockPos) {
        if (!this.selfSafety.getValue().booleanValue()) {
            return true;
        }
        Box box = new Box(blockPos);
        boolean intersects = minecraftClient.player.getBoundingBox().intersects(box);
        if (this.extrapolation.getValue().booleanValue()) {
            int intValue = this.ticks.getValue().intValue();
            while (true) {
                if (intValue < 0) {
                    break;
                }
                if (BaritoneHelper_3.mainhandHelper_2.getBox1109(minecraftClient.player, SearchHelper.get231(minecraftClient.player, this.ticks)).intersects(box)) {
                    intersects = true;
                    break;
                }
                intValue--;
            }
        }
        return !intersects;
    }

    public PlayerEntity getPlayerEntity886() {
        return (PlayerEntity) minecraftClient.world.getPlayers().stream().filter(abstractClientPlayerEntity -> {
            if ((!this.onlyFish.getValue().booleanValue() || abstractClientPlayerEntity.isInSwimmingPose()) && abstractClientPlayerEntity != minecraftClient.player && !Objects.equals(abstractClientPlayerEntity.getName().getString(), minecraftClient.player.getGameProfile().getName())) {
                if (!BaritoneHelper_3.searchHelper4_14.is519(abstractClientPlayerEntity.getName().getString()) && abstractClientPlayerEntity.isAlive() && !abstractClientPlayerEntity.isDead() && ((!this.ignoreNakeds.getValue().booleanValue() || HoleSnapSearchHelper4.is2013((LivingEntity) abstractClientPlayerEntity)) && abstractClientPlayerEntity.distanceTo(minecraftClient.player) <= this.range.getValue().floatValue())) {
                    return true;
                }
            }
            return false;
        }).min(Comparator.comparing(abstractClientPlayerEntity2 -> {
            return Double.valueOf(abstractClientPlayerEntity2.squaredDistanceTo(minecraftClient.player));
        })).orElse(null);
    }
}
