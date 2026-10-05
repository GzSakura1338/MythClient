package me.mioclient.module.player;

import java.util.List;
import java.util.Objects;
import java.util.concurrent.TimeUnit;
import java.util.function.Predicate;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.KeyPearlMode;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.SearchHelper4_8;
import me.mioclient.SearchHelper_4;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.feature.MotionEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.entity.Entity;
import net.minecraft.entity.passive.CatEntity;
import net.minecraft.entity.passive.ParrotEntity;
import net.minecraft.entity.passive.TameableEntity;
import net.minecraft.entity.passive.WolfEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.Hand;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoTame.class */
public class AutoTame extends Module {
    public Setting<AutoTameMode> action;
    public Setting<Float> range;
    public Setting<Integer> frequency;
    public Setting<Float> delay;
    public Setting<Boolean> rotate;
    public Setting<Boolean> targets;
    public Setting<Boolean> dogs;
    public Setting<Boolean> cats;
    public Setting<Boolean> parrots;
    public final Stopwatch stopwatch;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoTame$AutoTameMode.class */
    private static enum AutoTameMode implements EnumSettingHelper {
        autoTameMode("Tame") {
            @Override
            public Vec3d getVec3d650(TameableEntity tameableEntity) {
                int i;
                if (tameableEntity.getOwner() != null || (i = FireworksHelper.get448(AutoTame.getPredicate1845(tameableEntity))) == -1) {
                    return null;
                }
                FireworksHelper.do456(i);
                PhaseESPSearchHelper4_2.do3049((Entity) tameableEntity, Hand.MAIN_HAND);
                return tameableEntity.getBoundingBox().getCenter();
            }
        },
        autoTameMode2("Sit") {
            @Override
            public Vec3d getVec3d650(TameableEntity tameableEntity) {
                if (tameableEntity.isInSittingPose() || tameableEntity.isSitting() || SearchHelper_4.minecraftClient.player != tameableEntity.getOwner() || tameableEntity.isBreedingItem(SearchHelper_4.minecraftClient.player.getMainHandStack())) {
                    return null;
                }
                PhaseESPSearchHelper4_2.do3049((Entity) tameableEntity, Hand.MAIN_HAND);
                return tameableEntity.getBoundingBox().getCenter();
            }
        },
        autoTameMode3("Stand") {
            @Override
            public Vec3d getVec3d650(TameableEntity tameableEntity) {
                if ((!tameableEntity.isInSittingPose() && !tameableEntity.isSitting()) || SearchHelper_4.minecraftClient.player != tameableEntity.getOwner() || tameableEntity.isBreedingItem(SearchHelper_4.minecraftClient.player.getMainHandStack())) {
                    return null;
                }
                PhaseESPSearchHelper4_2.do3049((Entity) tameableEntity, Hand.MAIN_HAND);
                return tameableEntity.getBoundingBox().getCenter();
            }
        };

        public final String name;

        AutoTameMode(String str2) {
            this.name = str2;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public Vec3d getVec3d650(TameableEntity tameableEntity) {
            return null;
        }
    }

    public AutoTame() {
        super("AutoTame", "Automatically interacts with tame-able entities.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
    }

    @Listen
    public void do388(MotionEvent motionEvent) {
        Vec3d vec3d650;
        if (motionEvent.getKeyPearlMode1472() == KeyPearlMode.Post) {
            return;
        }
        if (this.stopwatch.is418(this.delay.getValue().floatValue(), TimeUnit.SECONDS)) {
            int i = 0;
            for (Entity entity : minecraftClient.world.getEntities()) {
                if (entity instanceof TameableEntity) {
                    TameableEntity tameableEntity = (TameableEntity) entity;
                    if (is1844(tameableEntity) && entity.distanceTo(minecraftClient.player) <= this.range.getValue().floatValue() && (vec3d650 = this.action.getValue().getVec3d650(tameableEntity)) != null) {
                        i++;
                        if (this.rotate.getValue().booleanValue()) {
                            motionEvent.do2257(SearchHelper4_8.getFloatArray2484(vec3d650));
                        }
                        if (i >= this.frequency.getValue().intValue()) {
                            break;
                        }
                    }
                }
            }
            this.stopwatch.reset();
        }
    }

    public boolean is1844(TameableEntity tameableEntity) {
        return ((tameableEntity instanceof CatEntity) && this.cats.getValue().booleanValue()) || ((tameableEntity instanceof WolfEntity) && this.dogs.getValue().booleanValue()) || ((tameableEntity instanceof ParrotEntity) && this.parrots.getValue().booleanValue());
    }

    public static Predicate<ItemStack> getPredicate1845(TameableEntity tameableEntity) {
        if (tameableEntity instanceof WolfEntity) {
            return itemStack -> {
                return itemStack.isOf(Items.BONE);
            };
        }
        if (tameableEntity instanceof ParrotEntity) {
            return itemStack2 -> {
                return List.of(Items.WHEAT_SEEDS, Items.MELON_SEEDS, Items.PUMPKIN_SEEDS, Items.BEETROOT_SEEDS).contains(itemStack2.getItem());
            };
        }
        Objects.requireNonNull(tameableEntity);
        return tameableEntity::isBreedingItem;
    }
}
