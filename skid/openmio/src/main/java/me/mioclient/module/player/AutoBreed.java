package me.mioclient.module.player;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.SwitchBootstraps;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.TimeUnit;
import java.util.function.Predicate;
import me.mioclient.AutoBreedHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FireworksHelper;
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
import net.minecraft.entity.passive.AnimalEntity;
import net.minecraft.entity.passive.CatEntity;
import net.minecraft.entity.passive.DonkeyEntity;
import net.minecraft.entity.passive.HorseEntity;
import net.minecraft.entity.passive.ParrotEntity;
import net.minecraft.entity.passive.WolfEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.Hand;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoBreed.class */
public class AutoBreed extends Module {
    public Setting<Float> range;
    public Setting<Integer> frequency;
    public Setting<Float> delay;
    public Setting<Boolean> rotate;
    public Setting<Boolean> targets;
    public Setting<Boolean> dogs;
    public Setting<Boolean> cats;
    public Setting<Boolean> parrots;
    public Setting<Boolean> donkeys;
    public Setting<Boolean> horses;
    public Setting<Boolean> others;
    public final Stopwatch stopwatch;
    public final List<AutoBreedHelper> list;

    public AutoBreed() {
        super("AutoBreed", "Breeds animals in between each other automatically.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
        this.list = Collections.synchronizedList(new ArrayList());
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        this.list.clear();
    }

    @Listen
    public void onEvent2(HoleSnapEvent holeSnapEvent) {
        this.list.forEach((v0) -> {
            v0.do1457();
        });
        this.list.removeIf(autoBreedHelper -> {
            return autoBreedHelper.get1459() >= 6000;
        });
        if (this.stopwatch.is418(this.delay.getValue().floatValue(), TimeUnit.SECONDS)) {
            int i = 0;
            for (Entity animalEntity : minecraftClient.world.getEntities()) {
                if (animalEntity instanceof AnimalEntity) {
                    AnimalEntity animalEntity2 = (AnimalEntity) animalEntity;
                    Iterator<AutoBreedHelper> it = this.list.iterator();
                    while (true) {
                        if (!it.hasNext()) {
                            if (is1204(animalEntity2) && animalEntity2.distanceTo(minecraftClient.player) <= this.range.getValue().floatValue()) {
                                int i2 = FireworksHelper.get448(getPredicate1203(animalEntity2));
                                if (i2 == -1) {
                                    return;
                                }
                                Vec3d center = animalEntity2.getBoundingBox().getCenter();
                                if (center != null) {
                                    FireworksHelper.do456(i2);
                                    PhaseESPSearchHelper4_2.do3049((Entity) animalEntity2, Hand.MAIN_HAND);
                                    this.list.add(new AutoBreedHelper(animalEntity2));
                                    i++;
                                    if (this.rotate.getValue().booleanValue()) {
                                        BaritoneHelper_3.searchHelper4_8.do2477(SearchHelper4_8.getFloatArray2484(center), 5);
                                    }
                                    if (i >= this.frequency.getValue().intValue()) {
                                        break;
                                    }
                                } else {
                                    continue;
                                }
                            }
                        } else {
                            if (animalEntity2 == it.next().getAnimalEntity1458()) {
                                break;
                            }
                        }
                    }
                }
            }
            this.stopwatch.reset();
        }
    }

    public static Predicate<ItemStack> getPredicate1203(AnimalEntity animalEntity) {
        if (animalEntity instanceof ParrotEntity) {
            return itemStack -> {
                return List.of(Items.WHEAT_SEEDS, Items.MELON_SEEDS, Items.PUMPKIN_SEEDS, Items.BEETROOT_SEEDS).contains(itemStack.getItem());
            };
        }
        Objects.requireNonNull(animalEntity);
        return animalEntity::isBreedingItem;
    }

    public boolean is1204(AnimalEntity animalEntity) {
        if (animalEntity.isBaby()) {
            return false;
        }
        Objects.requireNonNull(animalEntity);
        int typeSwitchIndex = animalEntity instanceof WolfEntity ? 0
                : animalEntity instanceof CatEntity ? 1
                : animalEntity instanceof ParrotEntity ? 2
                : animalEntity instanceof DonkeyEntity ? 3
                : animalEntity instanceof HorseEntity ? 4
                : -1;
        switch (typeSwitchIndex) {
            case 0:
                return this.dogs.getValue().booleanValue();
            case 1:
                return this.cats.getValue().booleanValue();
            case 2:
                return this.parrots.getValue().booleanValue();
            case 3:
                return this.donkeys.getValue().booleanValue();
            case 4:
                return this.horses.getValue().booleanValue();
            default:
                return animalEntity.getBreedingAge() != 0 && this.others.getValue().booleanValue();
        }
    }
}
