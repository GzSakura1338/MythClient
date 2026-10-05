package net.minecraft.data.loot;

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.stream.IntStream;
import net.fabricmc.fabric.api.datagen.v1.loot.FabricBlockLootSubProvider;
import net.minecraft.advancements.predicates.BlockPredicate;
import net.minecraft.advancements.predicates.DataComponentMatchers;
import net.minecraft.advancements.predicates.EnchantmentPredicate;
import net.minecraft.advancements.predicates.ItemPredicate;
import net.minecraft.advancements.predicates.LocationPredicate;
import net.minecraft.advancements.predicates.MinMaxBounds;
import net.minecraft.advancements.predicates.StatePropertiesPredicate;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.core.HolderLookup;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.component.predicates.DataComponentPredicates;
import net.minecraft.core.component.predicates.EnchantmentsPredicate;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.ResourceKey;
import net.minecraft.util.StringRepresentable;
import net.minecraft.world.flag.FeatureFlagSet;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.Items;
import net.minecraft.world.item.enchantment.Enchantment;
import net.minecraft.world.item.enchantment.Enchantments;
import net.minecraft.world.level.ItemLike;
import net.minecraft.world.level.block.BeehiveBlock;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.CandleBlock;
import net.minecraft.world.level.block.CaveVines;
import net.minecraft.world.level.block.CopperGolemStatueBlock;
import net.minecraft.world.level.block.DoorBlock;
import net.minecraft.world.level.block.DoublePlantBlock;
import net.minecraft.world.level.block.FlowerPotBlock;
import net.minecraft.world.level.block.MossyCarpetBlock;
import net.minecraft.world.level.block.MultifaceBlock;
import net.minecraft.world.level.block.SegmentableBlock;
import net.minecraft.world.level.block.SlabBlock;
import net.minecraft.world.level.block.StemBlock;
import net.minecraft.world.level.block.state.properties.DoubleBlockHalf;
import net.minecraft.world.level.block.state.properties.Property;
import net.minecraft.world.level.block.state.properties.SlabType;
import net.minecraft.world.level.storage.loot.IntRange;
import net.minecraft.world.level.storage.loot.LootPool;
import net.minecraft.world.level.storage.loot.LootTable;
import net.minecraft.world.level.storage.loot.entries.LootItem;
import net.minecraft.world.level.storage.loot.entries.LootPoolEntryContainer;
import net.minecraft.world.level.storage.loot.entries.LootPoolSingletonContainer;
import net.minecraft.world.level.storage.loot.functions.ApplyBonusCount;
import net.minecraft.world.level.storage.loot.functions.ApplyExplosionDecay;
import net.minecraft.world.level.storage.loot.functions.CopyBlockState;
import net.minecraft.world.level.storage.loot.functions.CopyComponentsFunction;
import net.minecraft.world.level.storage.loot.functions.FunctionUserBuilder;
import net.minecraft.world.level.storage.loot.functions.LimitCount;
import net.minecraft.world.level.storage.loot.functions.SetItemCountFunction;
import net.minecraft.world.level.storage.loot.parameters.LootContextParams;
import net.minecraft.world.level.storage.loot.predicates.BonusLevelTableCondition;
import net.minecraft.world.level.storage.loot.predicates.ConditionUserBuilder;
import net.minecraft.world.level.storage.loot.predicates.ExplosionCondition;
import net.minecraft.world.level.storage.loot.predicates.LocationCheck;
import net.minecraft.world.level.storage.loot.predicates.LootItemBlockStatePropertyCondition;
import net.minecraft.world.level.storage.loot.predicates.LootItemCondition;
import net.minecraft.world.level.storage.loot.predicates.LootItemRandomChanceCondition;
import net.minecraft.world.level.storage.loot.predicates.MatchTool;
import net.minecraft.world.level.storage.loot.providers.number.BinomialDistributionGenerator;
import net.minecraft.world.level.storage.loot.providers.number.ConstantValue;
import net.minecraft.world.level.storage.loot.providers.number.NumberProvider;
import net.minecraft.world.level.storage.loot.providers.number.UniformGenerator;

/**
 * Access widened by fabric-data-generation-api-v1 to accessible
 */
public abstract class BlockLootSubProvider implements LootTableSubProvider, FabricBlockLootSubProvider {
	protected final HolderLookup.Provider registries;
	private final Set<Item> explosionResistant;
	private final FeatureFlagSet enabledFeatures;
	private final Map<ResourceKey<LootTable>, LootTable.Builder> map = new HashMap<>();
	protected static final float[] NORMAL_LEAVES_SAPLING_CHANCES = new float[]{0.05F, 0.0625F, 0.083333336F, 0.1F};
	private static final float[] NORMAL_LEAVES_STICK_CHANCES = new float[]{0.02F, 0.022222223F, 0.025F, 0.033333335F, 0.1F};

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootItemCondition.Builder hasSilkTouch() {
		return MatchTool.toolMatches(
			ItemPredicate.Builder.item()
				.withComponents(
					DataComponentMatchers.Builder.components()
						.partial(
							DataComponentPredicates.ENCHANTMENTS,
							EnchantmentsPredicate.enchantments(
								List.of(
									new EnchantmentPredicate(this.registries.lookupOrThrow(Registries.ENCHANTMENT).getOrThrow(Enchantments.SILK_TOUCH), MinMaxBounds.Ints.atLeast(1))
								)
							)
						)
						.build()
				)
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootItemCondition.Builder doesNotHaveSilkTouch() {
		return this.hasSilkTouch().invert();
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootItemCondition.Builder hasShears() {
		return MatchTool.toolMatches(ItemPredicate.Builder.item().of(this.registries.lookupOrThrow(Registries.ITEM), Items.SHEARS));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public final LootItemCondition.Builder hasShearsOrSilkTouch() {
		return this.hasShears().or(this.hasSilkTouch());
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public final LootItemCondition.Builder doesNotHaveShearsOrSilkTouch() {
		return this.hasShearsOrSilkTouch().invert();
	}

	protected BlockLootSubProvider(final Set<Item> explosionResistant, final FeatureFlagSet enabledFeatures, final HolderLookup.Provider registries) {
		this.explosionResistant = explosionResistant;
		this.enabledFeatures = enabledFeatures;
		this.registries = registries;
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public <T extends FunctionUserBuilder<T>> T applyExplosionDecay(final ItemLike type, final FunctionUserBuilder<T> builder) {
		return !this.explosionResistant.contains(type.asItem()) ? builder.apply(ApplyExplosionDecay.explosionDecay()) : builder.unwrap();
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public <T extends ConditionUserBuilder<T>> T applyExplosionCondition(final ItemLike type, final ConditionUserBuilder<T> builder) {
		return !this.explosionResistant.contains(type.asItem()) ? builder.when(ExplosionCondition.survivesExplosion()) : builder.unwrap();
	}

	public LootTable.Builder createSingleItemTable(final ItemLike drop) {
		return LootTable.lootTable()
			.withPool(this.applyExplosionCondition(drop, LootPool.lootPool().setRolls(ConstantValue.exactly(1.0F)).add(LootItem.lootTableItem(drop))));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public static LootTable.Builder createSelfDropDispatchTable(
		final Block original, final LootItemCondition.Builder condition, final LootPoolEntryContainer.Builder<?> entry
	) {
		return LootTable.lootTable()
			.withPool(LootPool.lootPool().setRolls(ConstantValue.exactly(1.0F)).add(LootItem.lootTableItem(original).when(condition).otherwise(entry)));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createSilkTouchDispatchTable(final Block original, final LootPoolEntryContainer.Builder<?> entry) {
		return createSelfDropDispatchTable(original, this.hasSilkTouch(), entry);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createShearsDispatchTable(final Block original, final LootPoolEntryContainer.Builder<?> entry) {
		return createSelfDropDispatchTable(original, this.hasShears(), entry);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createSilkTouchOrShearsDispatchTable(final Block original, final LootPoolEntryContainer.Builder<?> entry) {
		return createSelfDropDispatchTable(original, this.hasShearsOrSilkTouch(), entry);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createSingleItemTableWithSilkTouch(final Block original, final ItemLike drop) {
		return this.createSilkTouchDispatchTable(original, (LootPoolEntryContainer.Builder<?>)this.applyExplosionCondition(original, LootItem.lootTableItem(drop)));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createSingleItemTable(final ItemLike drop, final NumberProvider count) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.setRolls(ConstantValue.exactly(1.0F))
					.add((LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(drop, LootItem.lootTableItem(drop).apply(SetItemCountFunction.setCount(count))))
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createSingleItemTableWithSilkTouch(final Block original, final ItemLike drop, final NumberProvider count) {
		return this.createSilkTouchDispatchTable(
			original, (LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(original, LootItem.lootTableItem(drop).apply(SetItemCountFunction.setCount(count)))
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public final LootTable.Builder createSilkTouchOnlyTable(final ItemLike drop) {
		return LootTable.lootTable().withPool(LootPool.lootPool().when(this.hasSilkTouch()).setRolls(ConstantValue.exactly(1.0F)).add(LootItem.lootTableItem(drop)));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public final LootTable.Builder createPotFlowerItemTable(final ItemLike flower) {
		return LootTable.lootTable()
			.withPool(
				this.applyExplosionCondition(Blocks.FLOWER_POT, LootPool.lootPool().setRolls(ConstantValue.exactly(1.0F)).add(LootItem.lootTableItem(Blocks.FLOWER_POT)))
			)
			.withPool(this.applyExplosionCondition(flower, LootPool.lootPool().setRolls(ConstantValue.exactly(1.0F)).add(LootItem.lootTableItem(flower))));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createSlabItemTable(final Block slab) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.setRolls(ConstantValue.exactly(1.0F))
					.add(
						(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
							slab,
							LootItem.lootTableItem(slab)
								.apply(
									SetItemCountFunction.setCount(ConstantValue.exactly(2.0F))
										.when(
											LootItemBlockStatePropertyCondition.hasBlockStateProperties(slab)
												.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(SlabBlock.TYPE, SlabType.DOUBLE))
										)
								)
						)
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public <T extends Comparable<T> & StringRepresentable> LootTable.Builder createSinglePropConditionTable(
		final Block drop, final Property<T> property, final T value
	) {
		return LootTable.lootTable()
			.withPool(
				this.applyExplosionCondition(
					drop,
					LootPool.lootPool()
						.setRolls(ConstantValue.exactly(1.0F))
						.add(
							LootItem.lootTableItem(drop)
								.when(
									LootItemBlockStatePropertyCondition.hasBlockStateProperties(drop)
										.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(property, value))
								)
						)
				)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createNameableBlockEntityTable(final Block drop) {
		return LootTable.lootTable()
			.withPool(
				this.applyExplosionCondition(
					drop,
					LootPool.lootPool()
						.setRolls(ConstantValue.exactly(1.0F))
						.add(
							LootItem.lootTableItem(drop)
								.apply(CopyComponentsFunction.copyComponentsFromBlockEntity(LootContextParams.BLOCK_ENTITY).include(DataComponents.CUSTOM_NAME))
						)
				)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createShulkerBoxDrop(final Block shulkerBox) {
		return LootTable.lootTable()
			.withPool(
				this.applyExplosionCondition(
					shulkerBox,
					LootPool.lootPool()
						.setRolls(ConstantValue.exactly(1.0F))
						.add(
							LootItem.lootTableItem(shulkerBox)
								.apply(
									CopyComponentsFunction.copyComponentsFromBlockEntity(LootContextParams.BLOCK_ENTITY)
										.include(DataComponents.CUSTOM_NAME)
										.include(DataComponents.CONTAINER)
										.include(DataComponents.LOCK)
										.include(DataComponents.CONTAINER_LOOT)
								)
						)
				)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createCopperOreDrops(final Block block) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.createSilkTouchDispatchTable(
			block,
			(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
				block,
				LootItem.lootTableItem(Items.RAW_COPPER)
					.apply(SetItemCountFunction.setCount(UniformGenerator.between(2.0F, 5.0F)))
					.apply(ApplyBonusCount.addOreBonusCount(enchantments.getOrThrow(Enchantments.FORTUNE)))
			)
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createLapisOreDrops(final Block block) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.createSilkTouchDispatchTable(
			block,
			(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
				block,
				LootItem.lootTableItem(Items.LAPIS_LAZULI)
					.apply(SetItemCountFunction.setCount(UniformGenerator.between(4.0F, 9.0F)))
					.apply(ApplyBonusCount.addOreBonusCount(enchantments.getOrThrow(Enchantments.FORTUNE)))
			)
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createRedstoneOreDrops(final Block block) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.createSilkTouchDispatchTable(
			block,
			(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
				block,
				LootItem.lootTableItem(Items.REDSTONE)
					.apply(SetItemCountFunction.setCount(UniformGenerator.between(4.0F, 5.0F)))
					.apply(ApplyBonusCount.addUniformBonusCount(enchantments.getOrThrow(Enchantments.FORTUNE)))
			)
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createBannerDrop(final Block original) {
		return LootTable.lootTable()
			.withPool(
				this.applyExplosionCondition(
					original,
					LootPool.lootPool()
						.setRolls(ConstantValue.exactly(1.0F))
						.add(
							LootItem.lootTableItem(original)
								.apply(
									CopyComponentsFunction.copyComponentsFromBlockEntity(LootContextParams.BLOCK_ENTITY)
										.include(DataComponents.CUSTOM_NAME)
										.include(DataComponents.ITEM_NAME)
										.include(DataComponents.TOOLTIP_DISPLAY)
										.include(DataComponents.BANNER_PATTERNS)
										.include(DataComponents.RARITY)
								)
						)
				)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createBeeNestDrop(final Block original) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.when(this.hasSilkTouch())
					.setRolls(ConstantValue.exactly(1.0F))
					.add(
						LootItem.lootTableItem(original)
							.apply(CopyComponentsFunction.copyComponentsFromBlockEntity(LootContextParams.BLOCK_ENTITY).include(DataComponents.BEES))
							.apply(CopyBlockState.copyState(original).copy(BeehiveBlock.HONEY_LEVEL))
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createBeeHiveDrop(final Block original) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.setRolls(ConstantValue.exactly(1.0F))
					.add(
						LootItem.lootTableItem(original)
							.when(this.hasSilkTouch())
							.apply(CopyComponentsFunction.copyComponentsFromBlockEntity(LootContextParams.BLOCK_ENTITY).include(DataComponents.BEES))
							.apply(CopyBlockState.copyState(original).copy(BeehiveBlock.HONEY_LEVEL))
							.otherwise(LootItem.lootTableItem(original))
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createCaveVinesDrop(final Block original) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.add(LootItem.lootTableItem(Items.GLOW_BERRIES))
					.when(
						LootItemBlockStatePropertyCondition.hasBlockStateProperties(original)
							.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(CaveVines.BERRIES, true))
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createCopperGolemStatueBlock(final Block block) {
		return LootTable.lootTable()
			.withPool(
				this.applyExplosionCondition(
					block,
					LootPool.lootPool()
						.setRolls(ConstantValue.exactly(1.0F))
						.add(
							LootItem.lootTableItem(block)
								.apply(CopyComponentsFunction.copyComponentsFromBlockEntity(LootContextParams.BLOCK_ENTITY).include(DataComponents.CUSTOM_NAME))
								.apply(CopyBlockState.copyState(block).copy(CopperGolemStatueBlock.POSE))
						)
				)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createOreDrop(final Block original, final Item drop) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.createSilkTouchDispatchTable(
			original,
			(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
				original, LootItem.lootTableItem(drop).apply(ApplyBonusCount.addOreBonusCount(enchantments.getOrThrow(Enchantments.FORTUNE)))
			)
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createMushroomBlockDrop(final Block original, final ItemLike drop) {
		return this.createSilkTouchDispatchTable(
			original,
			(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
				original,
				LootItem.lootTableItem(drop)
					.apply(SetItemCountFunction.setCount(UniformGenerator.between(-6.0F, 2.0F)))
					.apply(LimitCount.limitCount(IntRange.lowerBound(0)))
			)
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createGrassDrops(final Block original) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.createShearsDispatchTable(
			original,
			(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
				original,
				LootItem.lootTableItem(Items.WHEAT_SEEDS)
					.when(LootItemRandomChanceCondition.randomChance(0.125F))
					.apply(ApplyBonusCount.addUniformBonusCount(enchantments.getOrThrow(Enchantments.FORTUNE), 2))
			)
		);
	}

	public LootTable.Builder createStemDrops(final Block block, final Item drop) {
		return LootTable.lootTable()
			.withPool(
				this.applyExplosionDecay(
					block,
					LootPool.lootPool()
						.setRolls(ConstantValue.exactly(1.0F))
						.add(
							LootItem.lootTableItem(drop)
								.apply(
									StemBlock.AGE.getPossibleValues(),
									age -> SetItemCountFunction.setCount(BinomialDistributionGenerator.binomial(3, (age + 1) / 15.0F))
										.when(
											LootItemBlockStatePropertyCondition.hasBlockStateProperties(block)
												.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(StemBlock.AGE, age.intValue()))
										)
								)
						)
				)
			);
	}

	public LootTable.Builder createAttachedStemDrops(final Block block, final Item drop) {
		return LootTable.lootTable()
			.withPool(
				this.applyExplosionDecay(
					block,
					LootPool.lootPool()
						.setRolls(ConstantValue.exactly(1.0F))
						.add(LootItem.lootTableItem(drop).apply(SetItemCountFunction.setCount(BinomialDistributionGenerator.binomial(3, 0.53333336F))))
				)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createShearsOnlyDrop(final ItemLike drop) {
		return LootTable.lootTable().withPool(LootPool.lootPool().setRolls(ConstantValue.exactly(1.0F)).when(this.hasShears()).add(LootItem.lootTableItem(drop)));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createShearsOrSilkTouchOnlyDrop(final ItemLike drop) {
		return LootTable.lootTable()
			.withPool(LootPool.lootPool().setRolls(ConstantValue.exactly(1.0F)).when(this.hasShearsOrSilkTouch()).add(LootItem.lootTableItem(drop)));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createMultifaceBlockDrops(final Block block, final LootItemCondition.Builder condition) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.add(
						(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
							block,
							LootItem.lootTableItem(block)
								.when(condition)
								.apply(
									Direction.values(),
									dir -> SetItemCountFunction.setCount(ConstantValue.exactly(1.0F), true)
										.when(
											LootItemBlockStatePropertyCondition.hasBlockStateProperties(block)
												.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(MultifaceBlock.getFaceProperty(dir), true))
										)
								)
								.apply(SetItemCountFunction.setCount(ConstantValue.exactly(-1.0F), true))
						)
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createMultifaceBlockDrops(final Block block) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.add(
						(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
							block,
							LootItem.lootTableItem(block)
								.apply(
									Direction.values(),
									dir -> SetItemCountFunction.setCount(ConstantValue.exactly(1.0F), true)
										.when(
											LootItemBlockStatePropertyCondition.hasBlockStateProperties(block)
												.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(MultifaceBlock.getFaceProperty(dir), true))
										)
								)
								.apply(SetItemCountFunction.setCount(ConstantValue.exactly(-1.0F), true))
						)
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createMossyCarpetBlockDrops(final Block block) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.add(
						(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
							block,
							LootItem.lootTableItem(block)
								.when(
									LootItemBlockStatePropertyCondition.hasBlockStateProperties(block)
										.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(MossyCarpetBlock.BASE, true))
								)
						)
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createLeavesDrops(final Block original, final Block sapling, final float... saplingChances) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.createSilkTouchOrShearsDispatchTable(
				original,
				((LootPoolSingletonContainer.Builder)this.applyExplosionCondition(original, LootItem.lootTableItem(sapling)))
					.when(BonusLevelTableCondition.bonusLevelFlatChance(enchantments.getOrThrow(Enchantments.FORTUNE), saplingChances))
			)
			.withPool(
				LootPool.lootPool()
					.setRolls(ConstantValue.exactly(1.0F))
					.when(this.doesNotHaveShearsOrSilkTouch())
					.add(
						((LootPoolSingletonContainer.Builder)this.applyExplosionDecay(
								original, LootItem.lootTableItem(Items.STICK).apply(SetItemCountFunction.setCount(UniformGenerator.between(1.0F, 2.0F)))
							))
							.when(BonusLevelTableCondition.bonusLevelFlatChance(enchantments.getOrThrow(Enchantments.FORTUNE), NORMAL_LEAVES_STICK_CHANCES))
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createOakLeavesDrops(final Block original, final Block sapling, final float... saplingChances) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.createLeavesDrops(original, sapling, saplingChances)
			.withPool(
				LootPool.lootPool()
					.setRolls(ConstantValue.exactly(1.0F))
					.when(this.doesNotHaveShearsOrSilkTouch())
					.add(
						((LootPoolSingletonContainer.Builder)this.applyExplosionCondition(original, LootItem.lootTableItem(Items.APPLE)))
							.when(
								BonusLevelTableCondition.bonusLevelFlatChance(enchantments.getOrThrow(Enchantments.FORTUNE), 0.005F, 0.0055555557F, 0.00625F, 0.008333334F, 0.025F)
							)
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createMangroveLeavesDrops(final Block block) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.createSilkTouchOrShearsDispatchTable(
			block,
			((LootPoolSingletonContainer.Builder)this.applyExplosionDecay(
					Blocks.MANGROVE_LEAVES, LootItem.lootTableItem(Items.STICK).apply(SetItemCountFunction.setCount(UniformGenerator.between(1.0F, 2.0F)))
				))
				.when(BonusLevelTableCondition.bonusLevelFlatChance(enchantments.getOrThrow(Enchantments.FORTUNE), NORMAL_LEAVES_STICK_CHANCES))
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createCropDrops(final Block original, final Item cropDrop, final Item seedDrop, final LootItemCondition.Builder isMaxAge) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		return this.applyExplosionDecay(
			original,
			LootTable.lootTable()
				.withPool(LootPool.lootPool().add(LootItem.lootTableItem(cropDrop).when(isMaxAge).otherwise(LootItem.lootTableItem(seedDrop))))
				.withPool(
					LootPool.lootPool()
						.when(isMaxAge)
						.add(
							LootItem.lootTableItem(seedDrop).apply(ApplyBonusCount.addBonusBinomialDistributionCount(enchantments.getOrThrow(Enchantments.FORTUNE), 0.5714286F, 3))
						)
				)
		);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createDoublePlantShearsDrop(final Block block) {
		return LootTable.lootTable()
			.withPool(LootPool.lootPool().when(this.hasShears()).add(LootItem.lootTableItem(block).apply(SetItemCountFunction.setCount(ConstantValue.exactly(2.0F)))));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createDoublePlantWithSeedDrops(final Block block, final Block drop) {
		HolderLookup.RegistryLookup<Block> blocks = this.registries.lookupOrThrow(Registries.BLOCK);
		LootPoolEntryContainer.Builder<?> dropEntry = LootItem.lootTableItem(drop)
			.apply(SetItemCountFunction.setCount(ConstantValue.exactly(2.0F)))
			.when(this.hasShears())
			.otherwise(
				((LootPoolSingletonContainer.Builder)this.applyExplosionCondition(block, LootItem.lootTableItem(Items.WHEAT_SEEDS)))
					.when(LootItemRandomChanceCondition.randomChance(0.125F))
			);
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.add(dropEntry)
					.when(
						LootItemBlockStatePropertyCondition.hasBlockStateProperties(block)
							.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(DoublePlantBlock.HALF, DoubleBlockHalf.LOWER))
					)
					.when(
						LocationCheck.checkLocation(
							LocationPredicate.Builder.location()
								.setBlock(
									BlockPredicate.Builder.block()
										.of(blocks, block)
										.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(DoublePlantBlock.HALF, DoubleBlockHalf.UPPER))
								),
							new BlockPos(0, 1, 0)
						)
					)
			)
			.withPool(
				LootPool.lootPool()
					.add(dropEntry)
					.when(
						LootItemBlockStatePropertyCondition.hasBlockStateProperties(block)
							.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(DoublePlantBlock.HALF, DoubleBlockHalf.UPPER))
					)
					.when(
						LocationCheck.checkLocation(
							LocationPredicate.Builder.location()
								.setBlock(
									BlockPredicate.Builder.block()
										.of(blocks, block)
										.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(DoublePlantBlock.HALF, DoubleBlockHalf.LOWER))
								),
							new BlockPos(0, -1, 0)
						)
					)
			);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createCandleDrops(final Block block) {
		return LootTable.lootTable()
			.withPool(
				LootPool.lootPool()
					.setRolls(ConstantValue.exactly(1.0F))
					.add(
						(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
							block,
							LootItem.lootTableItem(block)
								.apply(
									List.of(2, 3, 4),
									count -> SetItemCountFunction.setCount(ConstantValue.exactly(count.intValue()))
										.when(
											LootItemBlockStatePropertyCondition.hasBlockStateProperties(block)
												.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(CandleBlock.CANDLES, count.intValue()))
										)
								)
						)
					)
			);
	}

	public LootTable.Builder createSegmentedBlockDrops(final Block block) {
		return block instanceof SegmentableBlock segmentableBlock
			? LootTable.lootTable()
				.withPool(
					LootPool.lootPool()
						.setRolls(ConstantValue.exactly(1.0F))
						.add(
							(LootPoolEntryContainer.Builder<?>)this.applyExplosionDecay(
								block,
								LootItem.lootTableItem(block)
									.apply(
										IntStream.rangeClosed(1, 4).boxed().toList(),
										count -> SetItemCountFunction.setCount(ConstantValue.exactly(count.intValue()))
											.when(
												LootItemBlockStatePropertyCondition.hasBlockStateProperties(block)
													.setProperties(StatePropertiesPredicate.Builder.properties().hasProperty(segmentableBlock.getSegmentAmountProperty(), count.intValue()))
											)
									)
							)
						)
				)
			: noDrop();
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public static LootTable.Builder createCandleCakeDrops(final Block candle) {
		return LootTable.lootTable().withPool(LootPool.lootPool().setRolls(ConstantValue.exactly(1.0F)).add(LootItem.lootTableItem(candle)));
	}

	public static LootTable.Builder noDrop() {
		return LootTable.lootTable();
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public abstract void generate();

	@Override
	public void generate(final BiConsumer<ResourceKey<LootTable>, LootTable.Builder> output) {
		this.generate();
		Set<ResourceKey<LootTable>> seen = new HashSet<>();

		for (Block block : BuiltInRegistries.BLOCK) {
			if (block.isEnabled(this.enabledFeatures)) {
				block.getLootTable()
					.ifPresent(
						lootTable -> {
							if (seen.add((ResourceKey<LootTable>)lootTable)) {
								LootTable.Builder builder = this.map.remove(lootTable);
								if (builder == null) {
									throw new IllegalStateException(
										String.format(Locale.ROOT, "Missing loottable '%s' for '%s'", lootTable.identifier(), BuiltInRegistries.BLOCK.getKey(block))
									);
								}

								output.accept((ResourceKey<LootTable>)lootTable, builder);
							}
						}
					);
			}
		}

		if (!this.map.isEmpty()) {
			throw new IllegalStateException("Created block loot tables for non-blocks: " + this.map.keySet());
		}
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public void addNetherVinesDropTable(final Block vineBlock, final Block plantBlock) {
		HolderLookup.RegistryLookup<Enchantment> enchantments = this.registries.lookupOrThrow(Registries.ENCHANTMENT);
		LootTable.Builder builder = this.createSilkTouchOrShearsDispatchTable(
			vineBlock,
			LootItem.lootTableItem(vineBlock)
				.when(BonusLevelTableCondition.bonusLevelFlatChance(enchantments.getOrThrow(Enchantments.FORTUNE), 0.33F, 0.55F, 0.77F, 1.0F))
		);
		this.add(vineBlock, builder);
		this.add(plantBlock, builder);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public LootTable.Builder createDoorTable(final Block block) {
		return this.createSinglePropConditionTable(block, DoorBlock.HALF, DoubleBlockHalf.LOWER);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public void dropPottedContents(final Block potted) {
		this.add(potted, block -> this.createPotFlowerItemTable(((FlowerPotBlock)block).getPotted()));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public void otherWhenSilkTouch(final Block block, final Block other) {
		this.add(block, this.createSilkTouchOnlyTable(other));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public void dropOther(final Block block, final ItemLike drop) {
		this.add(block, this.createSingleItemTable(drop));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public void dropWhenSilkTouch(final Block block) {
		this.otherWhenSilkTouch(block, block);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public void dropSelf(final Block block) {
		this.dropOther(block, block);
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public void add(final Block block, final Function<Block, LootTable.Builder> builder) {
		this.add(block, builder.apply(block));
	}

	/**
	 * Access widened by fabric-data-generation-api-v1 to accessible
	 */
	public void add(final Block block, final LootTable.Builder builder) {
		this.map.put(block.getLootTable().orElseThrow(() -> new IllegalStateException("Block " + block + " does not have loot table")), builder);
	}
}
