package net.minecraft.world.level.block;

import com.mojang.serialization.MapCodec;
import net.minecraft.world.level.block.state.BlockBehaviour;

/**
 * Access widened by fabric-transitive-access-wideners-v1 to accessible
 */
public class PlayerWallHeadBlock extends WallSkullBlock {
	public static final MapCodec<PlayerWallHeadBlock> CODEC = simpleCodec(PlayerWallHeadBlock::new);

	@Override
	public MapCodec<PlayerWallHeadBlock> codec() {
		return CODEC;
	}

	/**
	 * Access widened by fabric-transitive-access-wideners-v1 to accessible
	 */
	public PlayerWallHeadBlock(final BlockBehaviour.Properties properties) {
		super(SkullBlock.Types.PLAYER, properties);
	}
}
