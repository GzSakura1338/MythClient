package dev.myth.utils;

import net.minecraft.client.Minecraft;
import net.minecraft.util.Mth;
import net.minecraft.world.damagesource.CombatRules;
import net.minecraft.world.effect.MobEffects;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.ai.attributes.Attributes;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.item.enchantment.Enchantments;
import net.minecraft.world.level.ServerExplosion;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;

/** Mio ArmorSearchHelper4: end crystal explosion damage against armor and resistance. */
public final class CrystalDamage {
    /** End crystals explode with power 6; vanilla doubles it for the falloff radius. */
    public static final double CRYSTAL_RADIUS = 6;

    private CrystalDamage() {}

    public static float explosion(Vec3 center, LivingEntity entity) {
        return explosion(center, entity, false);
    }

    public static float explosion(Vec3 center, LivingEntity entity, boolean assumeBestArmor) {
        if (entity instanceof Player player && player.isCreative()) return 0;
        AABB box = entity.getBoundingBox();
        // Mio measures from the feet, horizontally centred, like vanilla's explosion falloff.
        Vec3 feet = new Vec3(Mth.lerp(.5, box.minX, box.maxX), box.minY, Mth.lerp(.5, box.minZ, box.maxZ));
        double diameter = CRYSTAL_RADIUS * 2;
        double distance = feet.distanceTo(center) / diameter;
        if (distance > 1) return 0;
        // getSeenPercent is the sampled exposure. Mio's decompiled copy divides two ints here,
        // which truncates every result to zero and is why that build never places.
        double exposure = (1 - distance) * ServerExplosion.getSeenPercent(center, entity);
        float raw = (float) ((exposure * exposure + exposure) / 2 * 7 * diameter + 1);
        return Math.max(reduce(entity, difficulty(raw), assumeBestArmor), 0);
    }

    /** Mio's armor threshold is the lowest remaining durability percent; an empty slot is zero. */
    public static int armorDurability(LivingEntity entity) {
        int worst = 100;
        for (EquipmentSlot slot : EquipmentSlot.values()) {
            if (slot.getType() != EquipmentSlot.Type.HUMANOID_ARMOR) continue;
            var stack = entity.getItemBySlot(slot);
            if (stack.isEmpty()) return 0;
            if (!stack.isDamageableItem() || stack.getMaxDamage() <= 0) continue;
            worst = Math.min(worst, Math.round((stack.getMaxDamage() - stack.getDamageValue()) * 100f / stack.getMaxDamage()));
        }
        return worst;
    }

    /** Armor and toughness, then Resistance, then the capped protection enchantment factor. */
    public static float reduce(LivingEntity entity, float damage) {
        return reduce(entity, damage, false);
    }

    public static float reduce(LivingEntity entity, float damage, boolean assumeBestArmor) {
        float left = CombatRules.getDamageAfterAbsorb(entity, damage, entity.damageSources().explosion(null),
                entity.getArmorValue(), (float) entity.getAttributeValue(Attributes.ARMOR_TOUGHNESS));
        var resistance = entity.getEffect(MobEffects.RESISTANCE);
        if (resistance != null) left = left * (25 - (resistance.getAmplifier() + 1) * 5) / 25f;
        int factor = assumeBestArmor ? 20 : Math.min(20, protection(entity));
        return Math.max(left * (1 - factor / 25f), 0);
    }

    /** Blast Protection counts double, matching Mio's enchantment protection factor. */
    public static int protection(LivingEntity entity) {
        int factor = 0;
        for (EquipmentSlot slot : EquipmentSlot.values()) {
            if (slot.getType() != EquipmentSlot.Type.HUMANOID_ARMOR) continue;
            var enchantments = entity.getItemBySlot(slot).getEnchantments();
            for (var enchantment : enchantments.keySet()) {
                int level = enchantments.getLevel(enchantment);
                if (enchantment.is(Enchantments.BLAST_PROTECTION)) factor += level * 2;
                else if (enchantment.is(Enchantments.PROTECTION)) factor += level;
            }
        }
        return factor;
    }

    public static float difficulty(float damage) {
        var level = Minecraft.getInstance().level;
        if (level == null) return damage;
        return switch (level.getDifficulty()) {
            case PEACEFUL -> 0;
            case EASY -> Math.min(damage / 2 + 1, damage);
            case HARD -> damage * 3 / 2;
            default -> damage;
        };
    }
}
