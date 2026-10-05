package today.starain.module.impl.misc;

import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.ambient.AmbientCreature;
import net.minecraft.world.entity.animal.Animal;
import net.minecraft.world.entity.animal.WaterAnimal;
import net.minecraft.world.entity.animal.horse.AbstractHorse;
import net.minecraft.world.entity.boss.EnderDragonPart;
import net.minecraft.world.entity.boss.enderdragon.EnderDragon;
import net.minecraft.world.entity.monster.Monster;
import net.minecraft.world.entity.monster.Slime;
import net.minecraft.world.entity.npc.AbstractVillager;
import net.minecraft.world.entity.player.Player;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.value.impl.BooleanValue;

/**
 * @Author：jiuxian_baka
 * @Date：2026/3/1 14:45
 */
public class Target extends ClientModule {

    private final BooleanValue player = new BooleanValue("Player", true, this);
    private final BooleanValue invisibles = new BooleanValue("Invisibles", false, this);
    private final BooleanValue animals = new BooleanValue("Animals", false, this);
    private final BooleanValue mobs = new BooleanValue("Mobs", false, this);
    private final BooleanValue villager = new BooleanValue("Villager", false, this);

    public Target() {
        setName("Target");
        setCategory(Category.MISC);
    }

    public static boolean isTarget(Entity entity) {
        if (!(entity instanceof LivingEntity)) return false;
        if (entity == mc.player) return false;
        if (entity instanceof Player && entity.isSpectator()) return false;
        if (!entity.isAlive()) return false;
        Target target = Starain.getInstance().getModuleManager().getModule(Target.class);

        if (entity.isInvisible() && !target.invisibles.get()) {
            return false;
        }

        if (entity instanceof Player) {
            return target.player.get();
        }

        if (entity instanceof AbstractVillager) {
            return target.villager.get();
        }

        if (entity instanceof Monster || entity instanceof Slime || entity instanceof EnderDragon || entity instanceof EnderDragonPart) {
            return target.mobs.get();
        }

        if (entity instanceof Animal || entity instanceof AmbientCreature || entity instanceof WaterAnimal || entity instanceof AbstractHorse) {
            return target.animals.get();
        }

        return false;
    }

    @Override
    public void onEnable() {
        this.setEnabled(false);
    }
}
