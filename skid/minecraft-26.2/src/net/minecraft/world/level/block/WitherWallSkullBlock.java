package net.minecraft.world.level.block;

import com.mojang.serialization.MapCodec;
import net.minecraft.core.BlockPos;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.block.state.BlockBehaviour;
import net.minecraft.world.level.block.state.BlockState;
import org.jspecify.annotations.Nullable;

/**
 * Access widened by fabric-transitive-access-wideners-v1 to accessible
 */
public class WitherWallSkullBlock extends WallSkullBlock {
	public static final MapCodec<WitherWallSkullBlock> CODEC = simpleCodec(WitherWallSkullBlock::new);

	@Override
	public MapCodec<WitherWallSkullBlock> codec() {
		return CODEC;
	}

	/**
	 * Access widened by fabric-transitive-access-wideners-v1 to accessible
	 */
	public WitherWallSkullBlock(final BlockBehaviour.Properties properties) {
		super(SkullBlock.Types.WITHER_SKELETON, properties);
	}

	@Override
	public void setPlacedBy(final Level level, final BlockPos pos, final BlockState state, final @Nullable LivingEntity by, final ItemStack itemStack) {
		WitherSkullBlock.checkSpawn(level, pos);
	}
}
