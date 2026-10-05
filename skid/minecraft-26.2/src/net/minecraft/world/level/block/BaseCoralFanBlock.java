package net.minecraft.world.level.block;

import com.mojang.serialization.MapCodec;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.BlockGetter;
import net.minecraft.world.level.block.state.BlockBehaviour;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.shapes.CollisionContext;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * Access widened by fabric-transitive-access-wideners-v1 to accessible
 */
public class BaseCoralFanBlock extends BaseCoralPlantTypeBlock {
	public static final MapCodec<BaseCoralFanBlock> CODEC = simpleCodec(BaseCoralFanBlock::new);
	private static final VoxelShape SHAPE = Block.column(12.0, 0.0, 4.0);

	@Override
	public MapCodec<? extends BaseCoralFanBlock> codec() {
		return CODEC;
	}

	/**
	 * Access widened by fabric-transitive-access-wideners-v1 to accessible
	 */
	public BaseCoralFanBlock(final BlockBehaviour.Properties properties) {
		super(properties);
	}

	@Override
	protected VoxelShape getShape(final BlockState state, final BlockGetter level, final BlockPos pos, final CollisionContext context) {
		return SHAPE;
	}
}
