package dev.myth.utils.inventory;

import dev.myth.utils.wrapper.Wrapper;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.component.DataComponents;
import net.minecraft.resources.ResourceKey;
import net.minecraft.tags.ItemTags;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.entity.ai.attributes.Attributes;
import net.minecraft.world.item.ArrowItem;
import net.minecraft.world.item.AxeItem;
import net.minecraft.world.item.BlockItem;
import net.minecraft.world.item.BowItem;
import net.minecraft.world.item.CrossbowItem;
import net.minecraft.world.item.ExperienceBottleItem;
import net.minecraft.world.item.FireworkRocketItem;
import net.minecraft.world.item.FishingRodItem;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.item.PlayerHeadItem;
import net.minecraft.world.item.ShovelItem;
import net.minecraft.world.item.enchantment.Enchantment;
import net.minecraft.world.item.enchantment.Enchantments;
import net.minecraft.world.item.enchantment.ItemEnchantments;
import net.minecraft.world.item.equipment.Equippable;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.BushBlock;
import net.minecraft.world.level.block.CropBlock;
import net.minecraft.world.level.block.FlowerBlock;
import net.minecraft.world.level.block.SkullBlock;
import net.minecraft.world.level.block.SlabBlock;

/**
 * Starain InventoryUtils. The scoring shape is kept as it was; what changed is how 26.2 answers
 * the questions. SwordItem, PickaxeItem, ArmorItem, TieredItem and ItemNameBlockItem are all gone,
 * so weapons and tools are matched by item tag and armor by its Equippable component. See
 * docs/starain-inventory-port.md for every formula that had to be re-derived.
 */
public final class InventoryUtils implements Wrapper {
    private InventoryUtils() {}

    /** Starain blacklistedBlocks, minus the duplicate entries it listed twice. */
    public static final List<Block> BLACKLISTED_BLOCKS = Arrays.asList(
            Blocks.AIR, Blocks.WATER, Blocks.LAVA, Blocks.ENCHANTING_TABLE, Blocks.GLASS_PANE,
            Blocks.IRON_BARS, Blocks.SNOW, Blocks.COAL_ORE, Blocks.DIAMOND_ORE, Blocks.EMERALD_ORE,
            Blocks.CHEST, Blocks.TRAPPED_CHEST, Blocks.TORCH, Blocks.ANVIL, Blocks.NOTE_BLOCK,
            Blocks.JUKEBOX, Blocks.TNT, Blocks.GOLD_ORE, Blocks.IRON_ORE, Blocks.LAPIS_ORE,
            Blocks.STONE_PRESSURE_PLATE, Blocks.LIGHT_WEIGHTED_PRESSURE_PLATE,
            Blocks.HEAVY_WEIGHTED_PRESSURE_PLATE, Blocks.STONE_BUTTON, Blocks.LEVER, Blocks.TALL_GRASS,
            Blocks.TRIPWIRE, Blocks.TRIPWIRE_HOOK, Blocks.RAIL, Blocks.CORNFLOWER, Blocks.RED_MUSHROOM,
            Blocks.BROWN_MUSHROOM, Blocks.VINE, Blocks.SUNFLOWER, Blocks.LADDER, Blocks.FURNACE,
            Blocks.SAND, Blocks.CACTUS, Blocks.DISPENSER, Blocks.DROPPER, Blocks.CRAFTING_TABLE,
            Blocks.COBWEB, Blocks.PUMPKIN, Blocks.COBBLESTONE_WALL, Blocks.OAK_FENCE,
            Blocks.REDSTONE_TORCH, Blocks.FLOWER_POT);

    /** InventoryMenu lays armor out as 5 HEAD .. 8 FEET, which is the order Starain indexed too. */
    public static final EquipmentSlot[] ARMOR_SLOTS =
            {EquipmentSlot.HEAD, EquipmentSlot.CHEST, EquipmentSlot.LEGS, EquipmentSlot.FEET};

    // ---------------------------------------------------------------- 26.2 lookups

    /**
     * 1.20.1 took a static Enchantment; 26.2 keys them by ResourceKey behind the registry. Reading
     * the stack's own component answers the same question without needing registry access, so this
     * stays usable with no level loaded.
     */
    public static int enchantLevel(ItemStack stack, ResourceKey<Enchantment> enchantment) {
        if (stack.isEmpty()) return 0;
        ItemEnchantments enchantments = stack.getOrDefault(DataComponents.ENCHANTMENTS, ItemEnchantments.EMPTY);
        for (var entry : enchantments.entrySet()) if (entry.getKey().is(enchantment)) return entry.getIntValue();
        return 0;
    }

    /** Sums one attribute over the stack's own modifiers, which is where 26.2 keeps the numbers. */
    public static double attribute(ItemStack stack, net.minecraft.core.Holder<net.minecraft.world.entity.ai.attributes.Attribute> attribute,
                                   EquipmentSlot slot) {
        if (stack.isEmpty()) return 0;
        double[] total = {0};
        stack.getOrDefault(DataComponents.ATTRIBUTE_MODIFIERS,
                net.minecraft.world.item.component.ItemAttributeModifiers.EMPTY)
                .forEach(slot, (holder, modifier) -> { if (holder == attribute) total[0] += modifier.amount(); });
        return total[0];
    }

    public static boolean isSword(ItemStack stack) { return stack.is(ItemTags.SWORDS); }
    public static boolean isPickaxe(ItemStack stack) { return stack.is(ItemTags.PICKAXES); }
    public static boolean isAxe(ItemStack stack) { return stack.is(ItemTags.AXES) || stack.getItem() instanceof AxeItem; }
    public static boolean isShovel(ItemStack stack) { return stack.is(ItemTags.SHOVELS) || stack.getItem() instanceof ShovelItem; }

    /** 26.2 has no ArmorItem, and the slot now comes off the Equippable component. */
    public static EquipmentSlot armorSlot(ItemStack stack) {
        if (stack.isEmpty()) return null;
        Equippable equippable = stack.get(DataComponents.EQUIPPABLE);
        if (equippable == null) return null;
        EquipmentSlot slot = equippable.slot();
        for (EquipmentSlot armor : ARMOR_SLOTS) if (armor == slot) return slot;
        return null;
    }

    public static boolean isArmor(ItemStack stack) { return armorSlot(stack) != null; }

    // ---------------------------------------------------------------- Starain predicates

    public static boolean shouldDisableFeatures() {
        return getAllItems().stream().anyMatch(item -> {
            if (item.isEmpty()) return false;
            String name = item.getDisplayName().getString();
            return name.contains("长按点击") || name.contains("点击使用") || name.contains("离开游戏")
                    || name.contains("选择一个队伍") || name.contains("再来一局");
        });
    }

    public static boolean isGoldenHead(ItemStack stack) {
        return !stack.isEmpty() && stack.getItem() instanceof BlockItem item && item.getBlock() instanceof SkullBlock;
    }

    public static boolean isSharpnessAxe(ItemStack stack) {
        if (stack.isEmpty() || !isAxe(stack)) return false;
        int level = enchantLevel(stack, Enchantments.SHARPNESS);
        return level >= 8 && level < 50;
    }

    public static boolean isGodAxe(ItemStack stack) {
        return !stack.isEmpty() && stack.getItem() == Items.GOLDEN_AXE
                && enchantLevel(stack, Enchantments.SHARPNESS) > 100;
    }

    public static boolean isEnchantedGApple(ItemStack stack) {
        return !stack.isEmpty() && stack.getItem() == Items.ENCHANTED_GOLDEN_APPLE;
    }

    public static boolean isEndCrystal(ItemStack stack) {
        return !stack.isEmpty() && stack.getItem() == Items.END_CRYSTAL;
    }

    public static boolean isKBBall(ItemStack stack) {
        return !stack.isEmpty() && stack.getItem() == Items.SLIME_BALL && enchantLevel(stack, Enchantments.KNOCKBACK) > 1;
    }

    public static boolean isKBStick(ItemStack stack) {
        return !stack.isEmpty() && stack.getItem() == Items.STICK && enchantLevel(stack, Enchantments.KNOCKBACK) > 1;
    }

    public static boolean isGodItem(ItemStack stack) {
        if (stack.isEmpty()) return false;
        if (isGodAxe(stack)) return true;
        if (stack.getItem() == Items.SLIME_BALL && enchantLevel(stack, Enchantments.KNOCKBACK) > 1) return true;
        return stack.getItem() == Items.TOTEM_OF_UNDYING || stack.getItem() == Items.END_CRYSTAL;
    }

    /** Starain's filter for server GUI items that must never be moved, thrown or counted. */
    public static boolean isItemValid(ItemStack stack) {
        if (stack.isEmpty()) return true;
        if (stack.getItem() instanceof PlayerHeadItem) return false;
        String name = stack.getDisplayName().getString();
        return !name.contains("Click") && !name.contains("Right") && !name.contains("点击")
                && !name.contains("Teleport") && !name.contains("使用") && !name.contains("传送")
                && !name.contains("再来");
    }

    public static boolean isCommonItemUseful(ItemStack stack) {
        if (stack.isEmpty()) return true;
        Item item = stack.getItem();
        if (item instanceof BlockItem block)
            return block.getBlock() != Blocks.ENCHANTING_TABLE && block.getBlock() != Blocks.COBWEB;
        if (item == Items.BOOK || item instanceof ExperienceBottleItem || item instanceof FireworkRocketItem) return false;
        if (item == Items.WHEAT_SEEDS || item == Items.BEETROOT_SEEDS || item == Items.MELON_SEEDS
                || item == Items.PUMPKIN_SEEDS) return false;
        return item != Items.FLINT_AND_STEEL;
    }

    /** A stack worth placing: a real block, more than one of it, and nothing decorative. */
    public static boolean isValidStack(ItemStack stack) {
        if (stack == null || !(stack.getItem() instanceof BlockItem blockItem) || stack.getCount() <= 1) return false;
        if (!isItemValid(stack)) return false;
        String name = stack.getDisplayName().getString();
        if (name.contains("Click") || name.contains("点击")) return false;
        Block block = blockItem.getBlock();
        // 26.2 dropped FungusBlock; mushrooms land on BushBlock, and both are blacklisted anyway.
        if (block instanceof FlowerBlock || block instanceof BushBlock || block instanceof CropBlock) return false;
        return !(block instanceof SlabBlock) && !BLACKLISTED_BLOCKS.contains(block);
    }

    // ---------------------------------------------------------------- slots

    public static List<ItemStack> getAllItems() {
        List<ItemStack> list = new ArrayList<>(40);
        if (mc.player == null) return list;
        list.addAll(mc.player.getInventory().getNonEquipmentItems());
        for (EquipmentSlot slot : ARMOR_SLOTS) list.add(mc.player.getItemBySlot(slot));
        return list;
    }

    /** The 36 main slots, hotbar first, exactly what Starain called Inventory.items. */
    public static List<ItemStack> items() {
        return mc.player == null ? List.of() : mc.player.getInventory().getNonEquipmentItems();
    }

    public static ItemStack item(int slot) {
        List<ItemStack> items = items();
        return slot < 0 || slot >= items.size() ? ItemStack.EMPTY : items.get(slot);
    }

    /** InventoryMenu puts the hotbar after the main inventory, so 0-8 map to 36-44. */
    public static int containerSlot(int inventorySlot) {
        return inventorySlot < 9 ? inventorySlot + 36 : inventorySlot;
    }

    public static int findEmptyInventory() {
        List<ItemStack> items = items();
        for (int i = 9; i < items.size(); i++) if (items.get(i).isEmpty()) return i;
        return -1;
    }

    public static int findEmptySlot() {
        for (int i = 0; i < 9; i++) if (item(i).isEmpty()) return i;
        return -1;
    }

    public static Integer findItemHotbar(Item item) {
        for (int i = 0; i < 9; i++) if (item(i).getItem() == item) return i;
        return null;
    }

    /** Starain compares by reference so a "best stack" maps back to the slot it came from. */
    public static int getItemStackSlot(ItemStack stack) {
        if (stack == null) return -1;
        List<ItemStack> items = items();
        for (int i = 0; i < items.size(); i++) if (items.get(i) == stack) return i;
        return -1;
    }

    public static int getItemSlot(Item item) {
        List<ItemStack> items = items();
        for (int i = 0; i < items.size(); i++) if (items.get(i).getItem() == item) return i;
        return -1;
    }

    public static boolean hasItem(Item item) {
        return getAllItems().stream().anyMatch(stack -> !stack.isEmpty() && stack.getItem() == item);
    }

    public static int getItemCount(Item item) {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && stack.getItem() == item)
                .mapToInt(ItemStack::getCount).sum();
    }

    // ---------------------------------------------------------------- scores

    /**
     * Starain scored armor as material tier * 100 plus the Protection level. 26.2 does not keep the
     * ArmorMaterial on the stack at all - it only exists while the item is being built - so the
     * tier is read back off the armor and toughness the piece actually grants. Same shape: the
     * material term dominates and Protection only breaks ties.
     */
    public static float getProtection(ItemStack stack) {
        if (stack == null || stack.isEmpty()) return 0;
        EquipmentSlot slot = armorSlot(stack);
        if (slot == null) return enchantLevel(stack, Enchantments.PROTECTION);
        double armor = attribute(stack, Attributes.ARMOR, slot);
        double toughness = attribute(stack, Attributes.ARMOR_TOUGHNESS, slot);
        return (float) (armor * 100 + toughness * 10) + enchantLevel(stack, Enchantments.PROTECTION);
    }

    public static float getBestArmorScore(EquipmentSlot slot) {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && armorSlot(stack) == slot)
                .map(InventoryUtils::getProtection).max(Float::compareTo).orElse(0f);
    }

    public static float getCurrentArmorScore(EquipmentSlot slot) {
        return mc.player == null ? 0 : getProtection(mc.player.getItemBySlot(slot));
    }

    /** Vanilla sharpness is AddValue(perLevel(1, 0.5)): 1 at level 1, +0.5 per level after. */
    public static float sharpnessBonus(ItemStack stack) {
        int level = enchantLevel(stack, Enchantments.SHARPNESS);
        return level <= 0 ? 0 : 1 + .5f * (level - 1);
    }

    /** 1.20.1 read SwordItem.getDamage() + 1; the same number now sits in the stack's modifiers. */
    public static float getSwordDamage(ItemStack stack) {
        if (stack == null || stack.isEmpty()) return 0;
        float damage = isSword(stack) ? (float) attribute(stack, Attributes.ATTACK_DAMAGE, EquipmentSlot.MAINHAND) + 1 : 0;
        return damage + sharpnessBonus(stack);
    }

    public static float getAxeDamage(ItemStack stack) {
        if (stack == null || stack.isEmpty()) return 0;
        float damage = isAxe(stack) && isSharpnessAxe(stack)
                ? (float) attribute(stack, Attributes.ATTACK_DAMAGE, EquipmentSlot.MAINHAND) + 1 : 0;
        return damage + sharpnessBonus(stack);
    }

    public static float getToolScore(ItemStack stack) {
        if (stack == null || stack.isEmpty() || isGodItem(stack) || isSharpnessAxe(stack)) return 0;
        float score;
        if (isPickaxe(stack)) score = stack.getDestroySpeed(Blocks.STONE.defaultBlockState());
        else if (isAxe(stack)) score = stack.getDestroySpeed(Blocks.OAK_LOG.defaultBlockState());
        else if (isShovel(stack)) score = stack.getDestroySpeed(Blocks.DIRT.defaultBlockState());
        else return 0;
        int efficiency = enchantLevel(stack, Enchantments.EFFICIENCY);
        return efficiency > 0 ? score + efficiency * .0075f : score;
    }

    public static float getPunchBowScore(ItemStack stack) {
        if (stack == null || stack.isEmpty() || !(stack.getItem() instanceof BowItem)) return 0;
        float score = 10;
        score += enchantLevel(stack, Enchantments.PUNCH);
        score += enchantLevel(stack, Enchantments.INFINITY);
        score += enchantLevel(stack, Enchantments.FLAME);
        score += enchantLevel(stack, Enchantments.POWER) / 10f;
        return score + durabilityTerm(stack);
    }

    public static float getPowerBowScore(ItemStack stack) {
        if (stack == null || stack.isEmpty() || !(stack.getItem() instanceof BowItem)) return 0;
        float score = 10;
        score += enchantLevel(stack, Enchantments.PUNCH) / 10f;
        score += enchantLevel(stack, Enchantments.INFINITY);
        score += enchantLevel(stack, Enchantments.FLAME);
        score += enchantLevel(stack, Enchantments.POWER);
        return score + durabilityTerm(stack);
    }

    /** Starain's trailing damage/maxDamage term, guarded because 26.2 allows maxDamage 0. */
    private static float durabilityTerm(ItemStack stack) {
        int max = stack.getMaxDamage();
        return max <= 0 ? 0 : (float) stack.getDamageValue() / max;
    }

    public static float getCrossbowScore(ItemStack stack) {
        if (stack == null || stack.isEmpty() || !(stack.getItem() instanceof CrossbowItem)) return 0;
        return enchantLevel(stack, Enchantments.QUICK_CHARGE)
                + enchantLevel(stack, Enchantments.MULTISHOT)
                + enchantLevel(stack, Enchantments.PIERCING);
    }

    public static boolean isPunchBow(ItemStack stack) { return getPunchBowScore(stack) > 10 && isItemValid(stack); }
    public static boolean isPowerBow(ItemStack stack) { return getPowerBowScore(stack) > 10 && isItemValid(stack); }

    // ---------------------------------------------------------------- best / worst

    private static ItemStack best(java.util.function.Predicate<ItemStack> filter,
                                  java.util.function.ToDoubleFunction<ItemStack> score) {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && filter.test(stack))
                .max(Comparator.comparingInt(stack -> (int) (score.applyAsDouble(stack) * 100))).orElse(null);
    }

    private static float bestScore(java.util.function.Predicate<ItemStack> filter,
                                   java.util.function.Function<ItemStack, Float> score) {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && filter.test(stack))
                .map(score).max(Float::compareTo).orElse(0f);
    }

    public static float getBestSwordDamage() { return bestScore(InventoryUtils::isSword, InventoryUtils::getSwordDamage); }
    public static ItemStack getBestSword() { return best(InventoryUtils::isSword, InventoryUtils::getSwordDamage); }

    public static float getBestPickaxeScore() {
        return bestScore(stack -> isPickaxe(stack) && isItemValid(stack), InventoryUtils::getToolScore);
    }
    public static ItemStack getBestPickaxe() {
        return best(stack -> isPickaxe(stack) && isItemValid(stack), InventoryUtils::getToolScore);
    }

    public static float getBestAxeScore() {
        return bestScore(stack -> isAxe(stack) && !isSharpnessAxe(stack) && isItemValid(stack), InventoryUtils::getToolScore);
    }
    public static ItemStack getBestAxe() {
        return best(stack -> isAxe(stack) && !isSharpnessAxe(stack) && isItemValid(stack), InventoryUtils::getToolScore);
    }
    /** Starain getBestShapeAxe: the sharpness axe used as a weapon, god axes excluded. */
    public static ItemStack getBestShapeAxe() {
        return best(stack -> isAxe(stack) && isSharpnessAxe(stack) && isItemValid(stack) && !isGodAxe(stack),
                InventoryUtils::getAxeDamage);
    }

    public static float getBestShovelScore() {
        return bestScore(stack -> isShovel(stack) && isItemValid(stack), InventoryUtils::getToolScore);
    }
    public static ItemStack getBestShovel() {
        return best(stack -> isShovel(stack) && isItemValid(stack), InventoryUtils::getToolScore);
    }

    public static float getBestCrossbowScore() {
        return bestScore(stack -> stack.getItem() instanceof CrossbowItem && isItemValid(stack), InventoryUtils::getCrossbowScore);
    }
    public static ItemStack getBestCrossbow() {
        return best(stack -> stack.getItem() instanceof CrossbowItem && isItemValid(stack), InventoryUtils::getCrossbowScore);
    }

    public static float getBestPunchBowScore() {
        return bestScore(stack -> stack.getItem() instanceof BowItem && isItemValid(stack), InventoryUtils::getPunchBowScore);
    }
    public static ItemStack getBestPunchBow() {
        return best(stack -> stack.getItem() instanceof BowItem && isItemValid(stack), InventoryUtils::getPunchBowScore);
    }

    public static float getBestPowerBowScore() {
        return bestScore(stack -> stack.getItem() instanceof BowItem && isItemValid(stack), InventoryUtils::getPowerBowScore);
    }
    public static ItemStack getBestPowerBow() {
        return best(stack -> stack.getItem() instanceof BowItem && isItemValid(stack), InventoryUtils::getPowerBowScore);
    }

    public static ItemStack getFishingRod() {
        return getAllItems().stream().filter(stack -> !stack.isEmpty()
                && stack.getItem() instanceof FishingRodItem && isItemValid(stack)).findAny().orElse(null);
    }

    private static boolean isProjectile(ItemStack stack) {
        return stack.getItem() == Items.EGG || stack.getItem() == Items.SNOWBALL;
    }

    public static ItemStack getBestProjectile() {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && isProjectile(stack) && isItemValid(stack))
                .max(Comparator.comparingInt(ItemStack::getCount)).orElse(null);
    }

    public static ItemStack getWorstProjectile() {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && isProjectile(stack))
                .min(Comparator.comparingInt(ItemStack::getCount)).orElse(null);
    }

    public static ItemStack getWorstArrow() {
        return getAllItems().stream().filter(stack -> !stack.isEmpty()
                        && stack.getItem() instanceof ArrowItem && isItemValid(stack))
                .min(Comparator.comparingInt(ItemStack::getCount)).orElse(null);
    }

    private static boolean placeableBlock(ItemStack stack) {
        return stack.getItem() instanceof BlockItem && isValidStack(stack) && isItemValid(stack);
    }

    public static int getBlockCountInInventory() {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && placeableBlock(stack))
                .mapToInt(ItemStack::getCount).sum();
    }

    public static ItemStack getBestBlock() {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && placeableBlock(stack))
                .max(Comparator.comparingInt(ItemStack::getCount)).orElse(null);
    }

    public static ItemStack getWorstBlock() {
        return getAllItems().stream().filter(stack -> !stack.isEmpty() && placeableBlock(stack))
                .min(Comparator.comparingInt(ItemStack::getCount)).orElse(null);
    }
}
