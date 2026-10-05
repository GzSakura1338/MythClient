package me.mioclient.module.combat;

import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.AutoCraftMode;
import me.mioclient.AutoMineHelper;
import me.mioclient.AutoMineHelper_2;
import me.mioclient.AutoMineSearchHelper4;
import me.mioclient.AutoMineSearchHelper42;
import me.mioclient.AutoMineSearchHelper42_2;
import me.mioclient.AutoMineSearchHelper42_3;
import me.mioclient.AutoMineSearchHelper42_4;
import me.mioclient.AutoMineSearchHelper42_5;
import me.mioclient.AutoMineSearchHelper42_6;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.HoleSnapEvent;
import me.mioclient.Mode_12;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.SearchHelper4_7;
import me.mioclient.SearchHelper4_8;
import me.mioclient.SpeedMineHelper;
import me.mioclient.TooltipsSearchHelper4_2;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import me.mioclient.module.client.AntiCheat;
import me.mioclient.module.player.SpeedMine;
import net.minecraft.util.Formatting;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/AutoMine.class */
public final class AutoMine extends Module {
    public static final int num = 1500;
    public static final SpeedMine speedMine = (SpeedMine) BaritoneHelper_3.baritoneHelper_4.getModule117(SpeedMine.class);
    public static final AntiCheat antiCheat = (AntiCheat) BaritoneHelper_3.baritoneHelper_4.getModule117(AntiCheat.class);
    public Setting<Mode_12> logic;
    public Setting<Boolean> strictDirection;
    public Setting<Boolean> raytrace;
    public Setting<Boolean> rotate;
    public Setting<Boolean> ignoreNaked;
    public Setting<Boolean> targets;
    public Setting<Boolean> enderChests;
    public Setting<Boolean> burrow;
    public Setting<Boolean> fish;
    public Setting<Boolean> head;
    public Setting<Boolean> trapOverride;
    public Setting<Boolean> feet;
    public Setting<Boolean> safe;
    public Setting<Boolean> self;
    public Setting<Boolean> face;
    public Setting<Boolean> onlyCrawl;
    public Setting<Boolean> downPriority;
    public Setting<Integer> grimDelay;
    public final List<AutoMineHelper_2> list;
    public final AutoMineSearchHelper4 autoMineSearchHelper4;
    public final SpeedMineHelper speedMineHelper;
    public final AtomicBoolean atomicBoolean;
    public boolean flag;

    public AutoMine() {
        super("AutoMine", new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("Mines out traps for your enemies and farms obsidian. \n\u0001Requires SpeedMine module enabled."), Category.COMBAT, "combatmine", "autocity");
        PhaseESPHelper.do1351(this);
        this.list = List.of(new AutoMineSearchHelper42_4(this), new AutoMineSearchHelper42_6(this), new AutoMineSearchHelper42_2(this), new AutoMineSearchHelper42_3(this), new AutoMineSearchHelper42_5(this), new AutoMineSearchHelper42(this));
        this.autoMineSearchHelper4 = new AutoMineSearchHelper4(this);
        this.speedMineHelper = new SpeedMineHelper(this);
        this.atomicBoolean = new AtomicBoolean();
    }

    @Listen(get219= 300)
    public void onEvent2(HoleSnapEvent holeSnapEvent) {
        if (speedMine.isToggled()) {
            AutoMineHelper autoMineHelper = new AutoMineHelper();
            for (AutoMineHelper_2 autoMineHelper_2 : this.list) {
                if (autoMineHelper_2.is465()) {
                    autoMineHelper_2.do722(autoMineHelper);
                }
            }
            if ((!is2751() || autoMineHelper.get888() >= 1500) && !this.atomicBoolean.get()) {
                if (autoMineHelper.is2897()) {
                    speedMine.do1048();
                    return;
                }
                if (autoMineHelper.getBlockPos386() == null) {
                    if (autoMineHelper.getBlockPos2898() == null || autoMineHelper.getBlockPos2898().equals(this.speedMineHelper.getBlockPos386())) {
                        return;
                    }
                    autoMineHelper.do667(autoMineHelper.getBlockPos2898());
                    autoMineHelper.do2899((BlockPos) null);
                }
                if (is2754() && this.speedMineHelper.is1225() && autoMineHelper.get888() < 1500) {
                    return;
                }
                BlockPos blockPos386 = autoMineHelper.getBlockPos386();
                BlockPos blockPos2898 = autoMineHelper.getBlockPos2898();
                Direction direction2750 = getDirection2750(blockPos386);
                if (direction2750 == null || blockPos386.equals(speedMine.getBlockPos1053())) {
                    return;
                }
                if (is2754() && blockPos2898 != null && !blockPos2898.equals(blockPos386) && !this.speedMineHelper.is1225()) {
                    do2749(blockPos2898, direction2750, false);
                }
                if (!antiCheat.is238()) {
                    do2749(blockPos386, direction2750, true);
                    return;
                }
                this.atomicBoolean.set(true);
                TooltipsSearchHelper4_2 tooltipsSearchHelper4_2 = BaritoneHelper_3.tooltipsSearchHelper4_2;
                Runnable runnable = () -> {
                    this.atomicBoolean.set(false);
                    do2749(blockPos386, direction2750, true);
                };
                tooltipsSearchHelper4_2.do164(runnable, this.grimDelay.getValue().intValue());
            }
        }
    }

    public void do2749(BlockPos blockPos, Direction direction, boolean z) {
        Vec3d vec3d2430;
        if (this.rotate.getValue().booleanValue()) {
            float[] floatArray2484 = SearchHelper4_8.getFloatArray2484(blockPos.toCenterPos());
            if (this.raytrace.getValue().booleanValue() && (vec3d2430 = SearchHelper4_7.getVec3d2430(blockPos, AutoCraftMode.X8)) != null) {
                floatArray2484 = SearchHelper4_8.getFloatArray2484(vec3d2430);
            }
            BaritoneHelper_3.searchHelper4_8.do2477(floatArray2484, 100);
        }
        SpeedMine.flag = z;
        minecraftClient.interactionManager.attackBlock(blockPos, direction);
        SpeedMine.flag = false;
    }

    public Direction getDirection2750(BlockPos blockPos) {
        if (!this.strictDirection.getValue().booleanValue()) {
            return Direction.UP;
        }
        List<Direction> list3031 = PhaseESPSearchHelper4_2.getList3031(blockPos);
        if (list3031.isEmpty()) {
            return null;
        }
        return (Direction) list3031.getFirst();
    }

    public boolean is2751() {
        if (is2753()) {
            if (speedMine.getBlockPos1053() == null) {
                do2752(false);
            } else {
                do2752(!SearchHelper4_7.is2446(speedMine.getBlockPos1053()));
            }
        }
        return is2753();
    }

    public void do2752(boolean z) {
        this.flag = z;
    }

    public boolean is2753() {
        return this.flag;
    }

    public float get1965() {
        return speedMine.range.getValue().floatValue();
    }

    public boolean is2754() {
        return speedMine.extraBreak.getValue().booleanValue();
    }
}
