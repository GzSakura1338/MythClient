package me.mioclient;

import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;
import me.mioclient.HoleSnapHelper_2;
import me.mioclient.feature.IllegalConstructorCall;
import me.mioclient.module.combat.AutoCrystal;
import me.mioclient.module.combat.AutoMine;
import net.minecraft.enchantment.Enchantment;
import net.minecraft.enchantment.Enchantments;
import net.minecraft.entity.DamageUtil;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.attribute.EntityAttributes;
import net.minecraft.entity.effect.StatusEffects;
import net.minecraft.item.ItemStack;
import net.minecraft.registry.RegistryKey;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.Difficulty;
import net.minecraft.world.explosion.Explosion;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArmorSearchHelper4.class */
public class ArmorSearchHelper4 implements SearchHelper_4 {
    public static AutoCrystal autoCrystal = (AutoCrystal) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoCrystal.class);
    public static AutoMine autoMine = (AutoMine) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoMine.class);

    /* JADX INFO: Access modifiers changed from: package-private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/ArmorSearchHelper4$Inner.class */
    public static /* synthetic */ class Inner {
        public static final /* synthetic */ int[] intArr = new int[Difficulty.values().length];

        static {
            try {
                intArr[Difficulty.PEACEFUL.ordinal()] = 1;
            } catch (NoSuchFieldError e) {
            }
            try {
                intArr[Difficulty.EASY.ordinal()] = 2;
            } catch (NoSuchFieldError e2) {
            }
            try {
                intArr[Difficulty.HARD.ordinal()] = 3;
            } catch (NoSuchFieldError e3) {
            }
        }
    }

    public static float get1895(Vec3d vec3d, LivingEntity livingEntity) {
        return get1899(vec3d, livingEntity, Double.longBitsToDouble(4618441417868443648L), true, (BlockPos) null);
    }

    public static float get1896(Vec3d vec3d, LivingEntity livingEntity, Box box) {
        return get1900(vec3d, livingEntity, box, Double.longBitsToDouble(4618441417868443648L), true, (BlockPos) null, (BlockPos) null);
    }

    public static float get1897(Vec3d vec3d, LivingEntity livingEntity) {
        return get1898(vec3d, livingEntity, livingEntity.getBoundingBox());
    }

    public static float get1898(Vec3d vec3d, LivingEntity livingEntity, Box box) {
        return get1900(vec3d, livingEntity, box, Double.longBitsToDouble(4617315517961601024L), true, (BlockPos) null, (BlockPos) null);
    }

    public static float get1899(Vec3d vec3d, LivingEntity livingEntity, double d, boolean z, BlockPos blockPos) {
        return get1900(vec3d, livingEntity, livingEntity.getBoundingBox(), d, z, blockPos, (BlockPos) null);
    }

    public static float get1900(Vec3d vec3d, LivingEntity livingEntity, Box box, double d, boolean z, BlockPos blockPos, BlockPos blockPos2) {
        if (livingEntity.equals(minecraftClient.player) && minecraftClient.player.isCreative()) {
            return 0.0f;
        }
        Vec3d vec3d2 = new Vec3d(MathHelper.lerp(FreecamHelper.val2, box.minX, box.maxX), box.minY, MathHelper.lerp(FreecamHelper.val2, box.minZ, box.maxZ));
        double longBitsToDouble = d * Double.longBitsToDouble(4611686018427387904L);
        double distanceTo = vec3d2.distanceTo(vec3d) / longBitsToDouble;
        if (distanceTo > Double.longBitsToDouble(4607182418800017408L)) {
            return 0.0f;
        }
        double longBitsToDouble2 = (Double.longBitsToDouble(4607182418800017408L) - distanceTo) * get1902(vec3d, (Entity) livingEntity, box, blockPos, blockPos2, z);
        return Math.max(get1903(livingEntity, get1904((float) (((((longBitsToDouble2 * longBitsToDouble2) + longBitsToDouble2) / Double.longBitsToDouble(4611686018427387904L)) * Double.longBitsToDouble(4619567317775286272L) * longBitsToDouble) + Double.longBitsToDouble(4607182418800017408L)))), 0.0f);
    }

    public static float get1901(Vec3d vec3d, Entity entity, BlockPos blockPos, boolean z) {
        return get1902(vec3d, entity, entity.getBoundingBox(), blockPos, (BlockPos) null, z);
    }

    public static float get1902(Vec3d vec3d, Entity entity, Box box, BlockPos blockPos, BlockPos blockPos2, boolean z) {
        double longBitsToDouble = Double.longBitsToDouble(4607182418800017408L) / (((box.maxX - box.minX) * Double.longBitsToDouble(4611686018427387904L)) + Double.longBitsToDouble(4607182418800017408L));
        double longBitsToDouble2 = Double.longBitsToDouble(4607182418800017408L) / (((box.maxY - box.minY) * Double.longBitsToDouble(4611686018427387904L)) + Double.longBitsToDouble(4607182418800017408L));
        double longBitsToDouble3 = Double.longBitsToDouble(4607182418800017408L) / (((box.maxZ - box.minZ) * Double.longBitsToDouble(4611686018427387904L)) + Double.longBitsToDouble(4607182418800017408L));
        double longBitsToDouble4 = (Double.longBitsToDouble(4607182418800017408L) - (Math.floor(Double.longBitsToDouble(4607182418800017408L) / longBitsToDouble) * longBitsToDouble)) / Double.longBitsToDouble(4611686018427387904L);
        double longBitsToDouble5 = (Double.longBitsToDouble(4607182418800017408L) - (Math.floor(Double.longBitsToDouble(4607182418800017408L) / longBitsToDouble3) * longBitsToDouble3)) / Double.longBitsToDouble(4611686018427387904L);
        if (longBitsToDouble < 0.0d || longBitsToDouble2 < 0.0d || longBitsToDouble3 < 0.0d) {
            return 0.0f;
        }
        int i = 0;
        int i2 = 0;
        double d = 0.0d;
        while (true) {
            double d2 = d;
            if (d2 > Double.longBitsToDouble(4607182418800017408L)) {
                return (float) i / (float) i2;   // 爆炸暴露率(命中样本/总样本), 必须浮点除法; 反编译丢了转型导致整数除法→恒0→水晶伤害≈0→AutoCrystal不放置
            }
            double d3 = 0.0d;
            while (true) {
                double d4 = d3;
                if (d4 <= Double.longBitsToDouble(4607182418800017408L)) {
                    double d5 = 0.0d;
                    while (true) {
                        double d6 = d5;
                        if (d6 <= Double.longBitsToDouble(4607182418800017408L)) {
                            HoleSnapHelper_2.Inner inner1603 = new HoleSnapHelper_2.Inner(new Vec3d(MathHelper.lerp(d2, box.minX, box.maxX) + longBitsToDouble4, MathHelper.lerp(d4, box.minY, box.maxY), MathHelper.lerp(d6, box.minZ, box.maxZ) + longBitsToDouble5), vec3d).getInner1603(entity);
                            HoleSnapHelper[] holeSnapHelperArr = new HoleSnapHelper[3];
                            BlockPos[] blockPosArr = new BlockPos[2];
                            blockPosArr[0] = blockPos;
                            blockPosArr[1] = blockPos == null ? null : autoMine.speedMineHelper.getBlockPos386();
                            holeSnapHelperArr[0] = HoleSnapHelper.getHoleSnapHelper1675(Arrays.asList(blockPosArr));
                            holeSnapHelperArr[1] = HoleSnapHelper.getHoleSnapHelper1676(blockPos2);
                            holeSnapHelperArr[2] = z ? HoleSnapHelper.holeSnapHelper2 : HoleSnapHelper.holeSnapHelper;
                            if (HoleSnapSearchHelper4_6.getBlockHitResult2784(inner1603.getInner1605(holeSnapHelperArr).getHoleSnapHelper_21606()).getType() == HitResult.Type.MISS) {
                                i++;
                            }
                            i2++;
                            d5 = d6 + longBitsToDouble3;
                        } else {
                            break;
                        }
                    }
                    d3 = d4 + longBitsToDouble2;
                } else {
                    break;
                }
            }
            d = d2 + longBitsToDouble;
        }
    }

    public static float get1903(LivingEntity livingEntity, float f) {
        float damageLeft = DamageUtil.getDamageLeft(livingEntity, f, minecraftClient.world.getDamageSources().explosion((Explosion) null), livingEntity.getArmor(), (float) livingEntity.getAttributeValue(EntityAttributes.GENERIC_ARMOR_TOUGHNESS));
        if (livingEntity.hasStatusEffect(StatusEffects.RESISTANCE)) {
            damageLeft = (damageLeft * (25 - ((livingEntity.getStatusEffect(StatusEffects.RESISTANCE).getAmplifier() + 1) * 5))) / Float.intBitsToFloat(1103626240);
        }
        int i = 0;
        Iterator it = livingEntity.getArmorItems().iterator();
        while (it.hasNext()) {
            Map<RegistryKey<Enchantment>, Integer> map1419 = IllegalConstructorCall.getMap1419((ItemStack) it.next());
            i += (map1419.getOrDefault(Enchantments.BLAST_PROTECTION, 0).intValue() * 2) + map1419.getOrDefault(Enchantments.PROTECTION, 0).intValue();
        }
        if (i > 20 || (autoCrystal.isToggled() && autoCrystal.assumeBestArmor.getValue().booleanValue())) {
            i = 20;
        }
        float intBitsToFloat = damageLeft * (Float.intBitsToFloat(1065353216) - (i / Float.intBitsToFloat(1103626240)));
        if (intBitsToFloat < 0.0f) {
            return 0.0f;
        }
        return intBitsToFloat;
    }

    public static float get1904(float f) {
        switch (Inner.intArr[minecraftClient.world.getDifficulty().ordinal()]) {
            case 1:
                return 0.0f;
            case 2:
                return Math.min((f / Float.intBitsToFloat(1073741824)) + Float.intBitsToFloat(1065353216), f);
            case 3:
                return (f * Float.intBitsToFloat(1077936128)) / Float.intBitsToFloat(1073741824);
            default:
                return f;
        }
    }

    public static int get1905(ItemStack itemStack) {
        return MathHelper.clamp(100 - ((int) Math.ceil((Float.intBitsToFloat(1065353216) - ((itemStack.getMaxDamage() - itemStack.getDamage()) / (float) itemStack.getMaxDamage())) * Float.intBitsToFloat(1120403456))), 1, 100);
    }
}
