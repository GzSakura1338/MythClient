package net.minecraft.world.level.block;

import com.mojang.serialization.MapCodec;
import net.minecraft.core.Direction;
import net.minecraft.world.level.block.state.BlockBehaviour;
import net.minecraft.world.level.block.state.BlockState;

/**
 * Access widened by fabric-transitive-access-wideners-v1 to accessible
 */
public class HalfTransparentBlock extends Block {
	public static final MapCodec<HalfTransparentBlock> CODEC = simpleCodec(HalfTransparentBlock::new);

	@Override
	protected MapCodec<? extends HalfTransparentBlock> codec() {
		return CODEC;
	}

	/**
	 * Access widened by fabric-transitive-access-wideners-v1 to accessible
	 */
	public HalfTransparentBlock(final BlockBehaviour.Properties properties) {
		super(properties);
	}

	@Override
	protected boolean skipRendering(final BlockState state, final BlockState neighborState, final Direction direction) {
		return neighborState.is(this) ? true : super.skipRendering(state, neighborState, direction);
	}
}
