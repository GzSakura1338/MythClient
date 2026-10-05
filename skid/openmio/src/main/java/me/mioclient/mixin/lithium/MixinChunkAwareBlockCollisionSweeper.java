package me.mioclient.mixin.lithium;

import me.mioclient.SearchHelper_4;
import me.mioclient.VoxelShapeEvent;
import net.minecraft.block.BlockState;
import net.minecraft.block.ShapeContext;
import net.minecraft.client.MinecraftClient;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.shape.VoxelShape;
import net.minecraft.util.shape.VoxelShapes;
import net.minecraft.world.BlockView;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Pseudo;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;

/* compiled from: 0.java */
@Pseudo
@Mixin(targets = {"me.jellysquid.mods.lithium.common.entity.movement.ChunkAwareBlockCollisionSweeper"}, remap = false)
/* loaded from: mio-yarn.jar:me/mioclient/mixin/lithium/MixinChunkAwareBlockCollisionSweeper.class */
public class MixinChunkAwareBlockCollisionSweeper {
    @Redirect(method = {"computeNext"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/block/BlockState;getCollisionShape(Lnet/minecraft/world/BlockView;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/ShapeContext;)Lnet/minecraft/util/shape/VoxelShape;"))
    private VoxelShape onComputeNextCollisionBox(BlockState blockState, BlockView blockView, BlockPos blockPos, ShapeContext shapeContext) {
        VoxelShape collisionShape = blockState.getCollisionShape(blockView, blockPos, shapeContext);
        if (blockView != MinecraftClient.getInstance().world) {
            return collisionShape;
        }
        VoxelShapeEvent voxelShapeEvent = (VoxelShapeEvent) SearchHelper_4.baritoneHelper.getObject1794(VoxelShapeEvent.getVoxelShapeEvent665(collisionShape, blockPos, blockState));
        return voxelShapeEvent.is2403() ? VoxelShapes.empty() : voxelShapeEvent.getVoxelShape669();
    }
}
