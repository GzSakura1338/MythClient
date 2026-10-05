package me.mioclient.module.player;

import java.util.concurrent.TimeUnit;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.HoleSnapEvent;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.SearchHelper4_8;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.Mount;
import net.minecraft.entity.Saddleable;
import net.minecraft.entity.passive.AbstractHorseEntity;
import net.minecraft.entity.vehicle.VehicleEntity;
import net.minecraft.util.Hand;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoMount.class */
public final class AutoMount extends Module {
    public Setting<Double> range;
    public Setting<Float> delay;
    public Setting<Boolean> rotate;
    public final Stopwatch stopwatch;

    public AutoMount() {
        super("AutoMount", "Mounts rideable entities automatically.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
    }

    @Listen
    public void onEvent2(HoleSnapEvent holeSnapEvent) {
        if (minecraftClient.player.hasVehicle()) {
            return;
        }
        if (this.stopwatch.is418(this.delay.getValue().floatValue(), TimeUnit.SECONDS)) {
            for (Entity livingEntity : minecraftClient.world.getEntities()) {
                if (minecraftClient.player.distanceTo((Entity) livingEntity) <= this.range.getValue().doubleValue() && (!(livingEntity instanceof LivingEntity) || !((LivingEntity) livingEntity).isBaby())) {
                    if (!(livingEntity instanceof Saddleable) || ((Saddleable) livingEntity).isSaddled() || (livingEntity instanceof AbstractHorseEntity)) {
                        if ((livingEntity instanceof Mount) || (livingEntity instanceof VehicleEntity)) {
                            PhaseESPSearchHelper4_2.do3049((Entity) livingEntity, Hand.MAIN_HAND);
                            if (this.rotate.getValue().booleanValue()) {
                                BaritoneHelper_3.searchHelper4_8.do2477(SearchHelper4_8.getFloatArray2484(((Entity) livingEntity).getBoundingBox().getCenter()), 5);
                            }
                            this.stopwatch.reset();
                            return;
                        }
                    }
                }
            }
        }
    }
}
