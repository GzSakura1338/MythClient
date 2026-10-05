package me.mioclient;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import me.mioclient.Feature_14;
import me.mioclient.api.Setting;
import me.mioclient.mixin.ducks.DuckLivingEntity;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.shape.VoxelShape;
import net.minecraft.util.shape.VoxelShapes;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper.class */
public class SearchHelper implements SearchHelper_4 {
    public static double val = Double.longBitsToDouble(4589168020290535424L);

    public static Vec3d getVec3d222(Box box) {
        return new Vec3d(MathHelper.lerp(FreecamHelper.val2, box.minX, box.maxX), box.minY, MathHelper.lerp(FreecamHelper.val2, box.minZ, box.maxZ));
    }

    public static Vec3d getVec3d223(Box box) {
        return getVec3d222(box).add(0.0d, box.getLengthY() * Double.longBitsToDouble(4598175219545276416L), 0.0d);
    }

    public static Vec3d getVec3d224(Box box) {
        return new Vec3d(MathHelper.lerp(FreecamHelper.val2, box.minX, box.maxX), box.maxY, MathHelper.lerp(FreecamHelper.val2, box.minZ, box.maxZ));
    }

    public static Box getBox225(Box box, Vec3d vec3d) {
        double lengthX = box.getLengthX() * FreecamHelper.val2;
        double lengthY = box.getLengthY() * FreecamHelper.val2;
        double lengthZ = box.getLengthZ() * FreecamHelper.val2;
        return new Box(vec3d.x - lengthX, vec3d.y - lengthY, vec3d.z - lengthZ, vec3d.x + lengthX, vec3d.y + lengthY, vec3d.z + lengthZ);
    }

    public static Box getBox226(Box box, BlockPos blockPos) {
        return getBox225(box, new Vec3d(blockPos.getX() + FreecamHelper.val2, blockPos.getY() + (box.getLengthY() * FreecamHelper.val2), blockPos.getZ() + FreecamHelper.val2));
    }

    public static Box getBox227(BlockPos blockPos) {
        BlockState blockState = minecraftClient.world.getBlockState(blockPos);
        if (blockState.isAir() || blockState.isOf(Blocks.AIR)) {
            return new Box(blockPos);
        }
        VoxelShape outlineShape = blockState.getOutlineShape(minecraftClient.world, blockPos);
        if (outlineShape.isEmpty()) {
            outlineShape = VoxelShapes.cuboid(0.0d, 0.0d, 0.0d, Double.longBitsToDouble(4607182418800017408L), Double.longBitsToDouble(4607182418800017408L), Double.longBitsToDouble(4607182418800017408L));
        }
        return outlineShape.getBoundingBox().offset(blockPos);
    }

    public static List<Vec3d> getList228(Box box, double d) {
        ArrayList<Vec3d> arrayList = new ArrayList<Vec3d>(List.of(new Vec3d(box.minX, d, box.minZ), new Vec3d(box.maxX, d, box.minZ), new Vec3d(box.minX, d, box.maxZ), new Vec3d(box.maxX, d, box.maxZ)));
        arrayList.sort(Comparator.comparing(vec3d -> {
            return Double.valueOf(minecraftClient.player.getEyePos().squaredDistanceTo(vec3d));
        }));
        return arrayList;
    }

    public static boolean is229(Box box) {
        return box.getLengthZ() == Double.longBitsToDouble(4607182418800017408L) && box.getLengthY() == Double.longBitsToDouble(4607182418800017408L) && box.getLengthX() == Double.longBitsToDouble(4607182418800017408L);
    }

    public static boolean is230(VoxelShape voxelShape) {
        if (voxelShape.isEmpty()) {
            return false;
        }
        return is229(voxelShape.getBoundingBox());
    }

    public static int get231(PlayerEntity playerEntity, Setting<Integer> setting) {
        if (!setting.is2348()) {
            return setting.getValue().intValue();
        }
        double hypot = Math.hypot(playerEntity.getX() - playerEntity.prevX, playerEntity.getZ() - playerEntity.prevZ) * Double.longBitsToDouble(4626322717216342016L) * Double.longBitsToDouble(4615288898129284301L);
        if (hypot < Double.longBitsToDouble(4613937818241073152L)) {
            return 0;
        }
        if (hypot <= Double.longBitsToDouble(4621819117588971520L)) {
            return 1;
        }
        if (hypot <= Double.longBitsToDouble(4626322717216342016L)) {
            return 3;
        }
        return hypot <= Double.longBitsToDouble(4629137466983448576L) ? 4 : 5;
    }

    public static Vec3d getVec3d232(Vec3d vec3d, Box box) {
        return new Vec3d(MathHelper.clamp(vec3d.getX(), box.minX, box.maxX), MathHelper.clamp(vec3d.getY(), box.minY, box.maxY), MathHelper.clamp(vec3d.getZ(), box.minZ, box.maxZ));
    }

    public static Box getBox233(Entity entity, float f) {
        return entity.getBoundingBox().offset(MathHelper.lerp(f, entity.lastRenderX, entity.getX()) - entity.getX(), MathHelper.lerp(f, entity.lastRenderY, entity.getY()) - entity.getY(), MathHelper.lerp(f, entity.lastRenderZ, entity.getZ()) - entity.getZ());
    }

    public static Box getBox234(LivingEntity livingEntity) {
        if (livingEntity instanceof Feature_14.OtherClientPlayerEntity) {
            return livingEntity.getBoundingBox();
        }
        Box boundingBox = livingEntity.getBoundingBox();
        DuckLivingEntity duckLivingEntity = (DuckLivingEntity) livingEntity;
        Vec3d vec3d = new Vec3d(duckLivingEntity.mio$getServerX(), duckLivingEntity.mio$getServerY(), duckLivingEntity.mio$getServerZ());
        if (vec3d.lengthSquared() == 0.0d) {
            return livingEntity.getBoundingBox();
        }
        double lengthX = boundingBox.getLengthX() / Double.longBitsToDouble(4611686018427387904L);
        double lengthZ = boundingBox.getLengthZ() / Double.longBitsToDouble(4611686018427387904L);
        return new Box(-lengthX, 0.0d, -lengthZ, lengthX, boundingBox.getLengthY(), lengthZ).offset(vec3d);
    }

    public static boolean is235(Box box, Box box2) {
        Box intersection = box.intersection(box2);
        return ((double) ((float) Math.min(intersection.getLengthX(), Math.min(intersection.getLengthY(), intersection.getLengthZ())))) > Double.longBitsToDouble(4502148214488346440L);
    }
}
