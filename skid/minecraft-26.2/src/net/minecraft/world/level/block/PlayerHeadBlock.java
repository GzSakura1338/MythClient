package net.minecraft.world.level.block;

import com.mojang.serialization.MapCodec;
import net.minecraft.world.level.block.state.BlockBehaviour;

/**
 * Access widened by fabric-transitive-access-wideners-v1 to accessible
 */
public class PlayerHeadBlock extends SkullBlock {
	public static final MapCodec<PlayerHeadBlock> CODEC = simpleCodec(PlayerHeadBlock::new);

	@Override
	public MapCodec<PlayerHeadBlock> codec() {
		return CODEC;
	}

	/**
	 * Access widened by fabric-transitive-access-wideners-v1 to accessible
	 */
	public PlayerHeadBlock(final BlockBehaviour.Properties properties) {
		super(SkullBlock.Types.PLAYER, properties);
	}
}
