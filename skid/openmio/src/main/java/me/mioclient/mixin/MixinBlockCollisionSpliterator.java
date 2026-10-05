package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.sugar.Local;
import me.mioclient.SearchHelper_4;
import me.mioclient.VoxelShapeEvent;
import net.minecraft.block.BlockState;
import net.minecraft.client.MinecraftClient;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.shape.VoxelShape;
import net.minecraft.util.shape.VoxelShapes;
import net.minecraft.world.BlockCollisionSpliterator;
import net.minecraft.world.CollisionView;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;

/* compiled from: 0.java */
@Mixin({BlockCollisionSpliterator.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBlockCollisionSpliterator.class */
public class MixinBlockCollisionSpliterator {

    @Shadow
    @Final
    private CollisionView field_25174;

    @Shadow
    @Final
    private BlockPos.Mutable field_25172;

    @ModifyExpressionValue(method = {"computeNext"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/block/BlockState;getCollisionShape(Lnet/minecraft/world/BlockView;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/ShapeContext;)Lnet/minecraft/util/shape/VoxelShape;")})
    private VoxelShape computeNextHook(VoxelShape voxelShape, @Local BlockState blockState) {
        if (this.field_25174 != MinecraftClient.getInstance().world) {
            return voxelShape;
        }
        VoxelShapeEvent voxelShapeEvent = (VoxelShapeEvent) SearchHelper_4.baritoneHelper.getObject1794(VoxelShapeEvent.getVoxelShapeEvent665(voxelShape, this.field_25172, blockState));
        return voxelShapeEvent.is2403() ? VoxelShapes.empty() : voxelShapeEvent.getVoxelShape669();
    }
}
