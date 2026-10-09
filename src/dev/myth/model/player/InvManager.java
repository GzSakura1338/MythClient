package dev.myth.model.player;

import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.MotionEvent;
import dev.myth.event.impl.PacketEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.TimerUtils;
import dev.myth.utils.inventory.InventoryUtils;
import dev.myth.utils.player.MoveUtils;
import dev.myth.utils.wrapper.Wrapper;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ThreadLocalRandom;
import net.minecraft.client.gui.screens.inventory.AbstractContainerScreen;
import net.minecraft.client.gui.screens.inventory.InventoryScreen;
import net.minecraft.network.protocol.game.ServerboundContainerClosePacket;
import net.minecraft.network.protocol.game.ServerboundInteractPacket;
import net.minecraft.network.protocol.game.ServerboundMovePlayerPacket;
import net.minecraft.network.protocol.game.ServerboundPlayerActionPacket;
import net.minecraft.network.protocol.game.ServerboundUseItemOnPacket;
import net.minecraft.network.protocol.game.ServerboundUseItemPacket;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.inventory.ContainerInput;

/**
 * Starain InvManager: sorts the hotbar into fixed slots, keeps the offhand stocked, equips the best
 * armor and throws away everything it does not want. Every click goes out as a real container
 * input against the player's own inventory menu, which is why it also has to close that menu again
 * whenever the server would otherwise see us moving or interacting with it open.
 */
public final class InvManager extends Model implements Wrapper {
    private static InvManager instance;
    private static final TimerUtils timer = new TimerUtils();

    public final Setting<Integer> minDelay = addSetting(Setting.integer("minDelay", "Min Delay", 125, 0, 1000).unit("ms"));
    public final Setting<Integer> delay = addSetting(Setting.integer("delay", "Delay", 150, 0, 1000).unit("ms"));
    public final Setting<String> offhandItems = addSetting(Setting.choice("offhandItems", "Offhand Items", "None",
            List.of("None", "Golden Apple", "Projectile", "Fishing Rod", "Block")));
    public final Setting<Boolean> autoArmor = addSetting(Setting.bool("autoArmor", "Auto Armor", true));
    public final Setting<Boolean> inventoryOnly = addSetting(Setting.bool("inventoryOnly", "Inventory Only", true));

    public final Setting<Boolean> switchSword = addSetting(Setting.bool("switchSword", "Switch Sword", true).group());
    public final Setting<Integer> swordSlot = addSetting(slot("swordSlot", "Sword Slot", 1, switchSword));
    public final Setting<Boolean> switchBlock = addSetting(Setting.bool("switchBlock", "Switch Block", true).group()
            .visibleWhen(() -> !isOffhand("Block")));
    public final Setting<Integer> blockSlot = addSetting(Setting.integer("blockSlot", "Block Slot", 2, 1, 9)
            .visibleWhen(() -> switchBlock.get() && switchBlock.isExpanded() && !isOffhand("Block")));
    public final Setting<Integer> maxBlockSize = addSetting(Setting.integer("maxBlockSize", "Max Block Size", 256, 64, 512)
            .step(64).visibleWhen(() -> switchBlock.get() && switchBlock.isExpanded()));
    public final Setting<Boolean> switchPickaxe = addSetting(Setting.bool("switchPickaxe", "Switch Pickaxe", true).group());
    public final Setting<Integer> pickaxeSlot = addSetting(slot("pickaxeSlot", "Pickaxe Slot", 3, switchPickaxe));
    public final Setting<Boolean> switchAxe = addSetting(Setting.bool("switchAxe", "Switch Axe", true).group());
    public final Setting<Integer> axeSlot = addSetting(slot("axeSlot", "Axe Slot", 4, switchAxe));
    public final Setting<Boolean> switchBow = addSetting(Setting.bool("switchBow", "Switch Bow or Crossbow", true).group());
    public final Setting<Integer> bowSlot = addSetting(slot("bowSlot", "Bow Slot", 5, switchBow));
    public final Setting<String> preferBow = addSetting(Setting.choice("preferBow", "Bow Priority", "Crossbow",
            List.of("Crossbow", "Power Bow", "Punch Bow")).visibleWhen(() -> switchBow.get() && switchBow.isExpanded()));
    public final Setting<Integer> maxArrowSize = addSetting(Setting.integer("maxArrowSize", "Max Arrow Size", 256, 64, 512)
            .step(64).visibleWhen(() -> switchBow.get() && switchBow.isExpanded()));
    public final Setting<Boolean> switchWaterBucket = addSetting(Setting.bool("switchWaterBucket", "Switch Water Bucket", true).group());
    public final Setting<Integer> waterBucketSlot = addSetting(slot("waterBucketSlot", "Water Bucket Slot", 6, switchWaterBucket));
    public final Setting<Boolean> switchEnderPearl = addSetting(Setting.bool("switchEnderPearl", "Switch Ender Pearl", true).group());
    public final Setting<Integer> enderPearlSlot = addSetting(slot("enderPearlSlot", "Ender Pearl Slot", 7, switchEnderPearl));
    public final Setting<Boolean> switchFireball = addSetting(Setting.bool("switchFireball", "Switch Fireball", true).group());
    public final Setting<Integer> fireballSlot = addSetting(slot("fireballSlot", "Fireball Slot", 8, switchFireball));
    public final Setting<Boolean> switchGoldenApple = addSetting(Setting.bool("switchGoldenApple", "Switch Golden Apple", true).group()
            .visibleWhen(() -> !isOffhand("Golden Apple")));
    public final Setting<Integer> goldenAppleSlot = addSetting(Setting.integer("goldenAppleSlot", "Golden Apple Slot", 9, 1, 9)
            .visibleWhen(() -> switchGoldenApple.get() && switchGoldenApple.isExpanded() && !isOffhand("Golden Apple")));

    public final Setting<Boolean> throwItems = addSetting(Setting.bool("throwItems", "Throw Items", true).group());
    public final Setting<Integer> waterBucketCount = addSetting(Setting.integer("waterBucketCount", "Keep Water Buckets", 1, 0, 5)
            .visibleWhen(() -> throwItems.get() && throwItems.isExpanded()));
    public final Setting<Integer> lavaBucketCount = addSetting(Setting.integer("lavaBucketCount", "Keep Lava Buckets", 1, 0, 5)
            .visibleWhen(() -> throwItems.get() && throwItems.isExpanded()));

    public final Setting<Boolean> keepProjectile = addSetting(Setting.bool("keepProjectile", "Keep Eggs & Snowballs", true).group());
    public final Setting<Boolean> switchProjectile = addSetting(Setting.bool("switchProjectile", "Switch Eggs & Snowballs", false)
            .visibleWhen(() -> keepProjectile.get() && keepProjectile.isExpanded() && !isOffhand("Projectile")));
    public final Setting<Integer> projectileSlot = addSetting(Setting.integer("projectileSlot", "Eggs & Snowballs Slot", 9, 1, 9)
            .visibleWhen(() -> keepProjectile.get() && keepProjectile.isExpanded() && switchProjectile.get() && !isOffhand("Projectile")));
    public final Setting<Integer> maxProjectileSize = addSetting(Setting.integer("maxProjectileSize", "Max Eggs & Snowballs Size", 64, 16, 256)
            .step(16).visibleWhen(() -> keepProjectile.get() && keepProjectile.isExpanded()));

    public final Setting<Boolean> switchRod = addSetting(Setting.bool("switchRod", "Switch Rod", false).group()
            .visibleWhen(() -> !isOffhand("Fishing Rod")));
    public final Setting<Integer> rodSlot = addSetting(Setting.integer("rodSlot", "Rod Slot", 9, 1, 9)
            .visibleWhen(() -> switchRod.get() && switchRod.isExpanded() && !isOffhand("Fishing Rod")));

    private int noMoveTicks;
    private boolean clickOffHand;
    private boolean inventoryOpen;

    public InvManager() {
        super("invmanager", "InvManager", Category.PLAYER);
        setDescription("Sorts the hotbar, equips the best armor and drops what is not wanted.");
        instance = this;
    }

    private Setting<Integer> slot(String id, String name, int value, Setting<Boolean> parent) {
        return Setting.integer(id, name, value, 1, 9).visibleWhen(() -> parent.get() && parent.isExpanded());
    }

    private boolean isOffhand(String mode) { return offhandItems.get().equals(mode); }

    // Starain reads these off the module instance whether or not it is enabled.
    public static int getMaxBlockSize() { return instance == null ? 256 : instance.maxBlockSize.get(); }
    public static boolean shouldKeepProjectile() { return instance != null && instance.keepProjectile.get(); }
    public static int getMaxProjectileSize() { return instance == null ? 64 : instance.maxProjectileSize.get(); }
    public static int getMaxArrowSize() { return instance == null ? 256 : instance.maxArrowSize.get(); }
    public static int getWaterBucketCount() { return instance == null ? 1 : instance.waterBucketCount.get(); }
    public static int getLavaBucketCount() { return instance == null ? 1 : instance.lavaBucketCount.get(); }

    @Override protected void onDisable() {
        noMoveTicks = 0;
        clickOffHand = false;
        inventoryOpen = false;
    }

    private int nextDelay() {
        return Math.max(minDelay.get(), (int) (delay.get() + ThreadLocalRandom.current().nextGaussian() * 50));
    }

    private boolean ready(int next) { return timer.elapsedMillis() >= next; }

    /**
     * Starain onPacket: the clicks above go out without the menu ever being opened, so the server
     * still thinks it is open. Anything that would be illegal with an open container closes it
     * first. Runs on the netty thread as well as the client one, so it only reads settings.
     */
    @EventTarget
    public void onPacket(PacketEvent event) {
        if (event.getType() != EventType.OUTGOING) return;
        if (event.getPacket() instanceof ServerboundContainerClosePacket) inventoryOpen = false;
        if (!inventoryOpen || inventoryOnly.get() || mc.player == null || mc.getConnection() == null) return;
        if (event.getPacket() instanceof ServerboundMovePlayerPacket) {
            if (MoveUtils.isMoving()) closeContainer();
        } else if (event.getPacket() instanceof ServerboundUseItemOnPacket
                || event.getPacket() instanceof ServerboundUseItemPacket
                || event.getPacket() instanceof ServerboundInteractPacket
                || event.getPacket() instanceof ServerboundPlayerActionPacket) {
            closeContainer();
        }
    }

    private void closeContainer() {
        mc.getConnection().send(new ServerboundContainerClosePacket(mc.player.inventoryMenu.containerId));
    }

    @EventTarget
    public void onMotion(MotionEvent event) {
        if (event.getType() != EventType.PRE || mc.player == null || mc.level == null || mc.gameMode == null) return;
        if (InventoryUtils.shouldDisableFeatures()) return;
        if (MoveUtils.isMoving()) noMoveTicks = 0;
        else noMoveTicks++;
        boolean allowMove = !inventoryOnly.get();
        if (ChestStealer.isWorking()
                || (inventoryOnly.get() ? !(mc.gui.screen() instanceof InventoryScreen) : (!allowMove && noMoveTicks <= 1))) {
            clickOffHand = false;
            return;
        }
        // Someone else's container is open: its slot ids are not the player menu's.
        if (mc.gui.screen() instanceof AbstractContainerScreen<?> container
                && container.getMenu().containerId != mc.player.inventoryMenu.containerId) return;
        int next = nextDelay();

        if (autoArmor.get()) {
            for (EquipmentSlot slot : InventoryUtils.ARMOR_SLOTS) {
                ItemStack worn = mc.player.getItemBySlot(slot);
                if (worn.isEmpty() || InventoryUtils.armorSlot(worn) == null) continue;
                if (ready(next) && InventoryUtils.getBestArmorScore(slot) > InventoryUtils.getProtection(worn)) {
                    click(armorContainerSlot(slot), 1, ContainerInput.THROW);
                }
            }
            List<ItemStack> items = InventoryUtils.items();
            for (int i = 0; i < items.size(); i++) {
                ItemStack stack = items.get(i);
                EquipmentSlot slot = InventoryUtils.armorSlot(stack);
                if (stack.isEmpty() || slot == null) continue;
                float score = InventoryUtils.getProtection(stack);
                boolean isBest = InventoryUtils.getBestArmorScore(slot) == score;
                boolean isBetter = InventoryUtils.getCurrentArmorScore(slot) < score;
                if (isBest && isBetter && ready(next)) click(InventoryUtils.containerSlot(i), 0, ContainerInput.QUICK_MOVE);
            }
        }

        if (clickOffHand && ready(next)) {
            click(45, 0, ContainerInput.PICKUP);
            clickOffHand = false;
        }

        ItemStack offHand = mc.player.getItemBySlot(EquipmentSlot.OFFHAND);
        if (isOffhand("Golden Apple")) {
            int egapSlot = InventoryUtils.getItemSlot(Items.ENCHANTED_GOLDEN_APPLE);
            int gapSlot = InventoryUtils.getItemSlot(Items.GOLDEN_APPLE);
            if (offHand.getItem() == Items.ENCHANTED_GOLDEN_APPLE) {
                if (offHand.getCount() < offHand.getMaxStackSize() && egapSlot != -1 && ready(next)) topUpOffHand(egapSlot);
            } else if (offHand.getItem() == Items.GOLDEN_APPLE) {
                if (egapSlot != -1) {
                    if (ready(next)) swapOffHand(egapSlot);
                } else if (offHand.getCount() < offHand.getMaxStackSize() && gapSlot != -1 && ready(next)) {
                    topUpOffHand(gapSlot);
                }
            } else if (egapSlot != -1) {
                if (ready(next)) swapOffHand(egapSlot);
            } else if (gapSlot != -1 && ready(next)) {
                swapOffHand(gapSlot);
            }
        } else if (isOffhand("Projectile")) {
            ItemStack best = InventoryUtils.getBestProjectile();
            if (best != null) {
                int slot = InventoryUtils.getItemStackSlot(best);
                boolean shouldSwap = offHand.getItem() != Items.EGG && offHand.getItem() != Items.SNOWBALL
                        || offHand.getCount() < best.getCount();
                if (shouldSwap && slot != -1 && ready(next)) swapOffHand(slot);
            }
        } else if (isOffhand("Fishing Rod")) {
            int slot = InventoryUtils.getItemSlot(Items.FISHING_ROD);
            if (slot != -1 && ready(next) && offHand.getItem() != Items.FISHING_ROD) swapOffHand(slot);
        } else if (isOffhand("Block")) {
            ItemStack best = InventoryUtils.getBestBlock();
            if (best != null) {
                int slot = InventoryUtils.getItemStackSlot(best);
                boolean shouldSwap = !InventoryUtils.isValidStack(offHand) || offHand.getCount() < best.getCount();
                if (shouldSwap && slot != -1 && ready(next)) swapOffHand(slot);
            }
        }

        if (switchGoldenApple.get() && !isOffhand("Golden Apple")) {
            int target = goldenAppleSlot.get() - 1;
            int egapSlot = InventoryUtils.getItemSlot(Items.ENCHANTED_GOLDEN_APPLE);
            int gapSlot = InventoryUtils.getItemSlot(Items.GOLDEN_APPLE);
            int bestSlot = egapSlot != -1 ? egapSlot : gapSlot;
            if (bestSlot != -1) {
                ItemStack current = InventoryUtils.item(target);
                ItemStack best = InventoryUtils.item(bestSlot);
                if (current.getItem() != best.getItem() || current.getCount() < best.getCount()) swapItem(target, best);
            }
        }

        if (switchBlock.get()) {
            int target = blockSlot.get() - 1;
            ItemStack current = InventoryUtils.item(target);
            ItemStack best = InventoryUtils.getBestBlock();
            if (best != null && (best.getCount() > current.getCount() || !InventoryUtils.isValidStack(current))
                    && !isOffhand("Block")) swapItem(target, best);
            if (InventoryUtils.getBlockCountInInventory() > maxBlockSize.get()) throwItem(InventoryUtils.getWorstBlock());
        }

        if (switchSword.get()) {
            int target = swordSlot.get() - 1;
            ItemStack current = InventoryUtils.item(target);
            ItemStack best = InventoryUtils.getBestSword();
            ItemStack shapeAxe = InventoryUtils.getBestShapeAxe();
            if (InventoryUtils.getAxeDamage(shapeAxe) > InventoryUtils.getSwordDamage(best)) best = shapeAxe;
            if (best != null) {
                float currentDamage = InventoryUtils.isSword(current)
                        ? InventoryUtils.getSwordDamage(current) : InventoryUtils.getAxeDamage(current);
                float bestDamage = InventoryUtils.isSword(best)
                        ? InventoryUtils.getSwordDamage(best) : InventoryUtils.getAxeDamage(best);
                if (bestDamage > currentDamage) swapItem(target, best);
            }
        }

        if (switchPickaxe.get()) {
            int target = pickaxeSlot.get() - 1;
            ItemStack best = InventoryUtils.getBestPickaxe();
            ItemStack current = InventoryUtils.item(target);
            if (best != null && InventoryUtils.isPickaxe(best)
                    && (InventoryUtils.getToolScore(best) > InventoryUtils.getToolScore(current) || !InventoryUtils.isPickaxe(current)))
                swapItem(target, best);
        }

        if (switchAxe.get()) {
            int target = axeSlot.get() - 1;
            ItemStack best = InventoryUtils.getBestAxe();
            ItemStack current = InventoryUtils.item(target);
            if (best != null && InventoryUtils.isAxe(best)
                    && (InventoryUtils.getToolScore(best) > InventoryUtils.getToolScore(current) || !InventoryUtils.isAxe(current)))
                swapItem(target, best);
        }

        if (switchRod.get() && !isOffhand("Fishing Rod")) {
            int target = rodSlot.get() - 1;
            ItemStack current = InventoryUtils.item(target);
            if (!(current.getItem() instanceof net.minecraft.world.item.FishingRodItem))
                swapItem(target, InventoryUtils.getFishingRod());
        }

        if (switchBow.get()) {
            int target = bowSlot.get() - 1;
            ItemStack current = InventoryUtils.item(target);
            ItemStack best;
            float bestScore, currentScore;
            if (preferBow.get().equals("Crossbow")) {
                best = InventoryUtils.getBestCrossbow();
                bestScore = InventoryUtils.getCrossbowScore(best);
                currentScore = InventoryUtils.getCrossbowScore(current);
            } else if (preferBow.get().equals("Power Bow")) {
                best = InventoryUtils.getBestPowerBow();
                bestScore = InventoryUtils.getPowerBowScore(best);
                currentScore = InventoryUtils.getPowerBowScore(current);
            } else {
                best = InventoryUtils.getBestPunchBow();
                bestScore = InventoryUtils.getPunchBowScore(best);
                currentScore = InventoryUtils.getPunchBowScore(current);
            }
            if (best == null) {
                best = InventoryUtils.getBestCrossbow();
                bestScore = InventoryUtils.getCrossbowScore(best);
                currentScore = InventoryUtils.getCrossbowScore(current);
            }
            if (best == null) {
                best = InventoryUtils.getBestPowerBow();
                bestScore = InventoryUtils.getPowerBowScore(best);
                currentScore = InventoryUtils.getPowerBowScore(current);
            }
            if (best == null) {
                best = InventoryUtils.getBestPunchBow();
                bestScore = InventoryUtils.getPunchBowScore(best);
                currentScore = InventoryUtils.getPunchBowScore(current);
            }
            if (best != null && bestScore > currentScore) swapItem(target, best);
            if (InventoryUtils.getItemCount(Items.ARROW) > maxArrowSize.get()) throwItem(InventoryUtils.getWorstArrow());
        }

        if (switchEnderPearl.get()) swapItem(enderPearlSlot.get() - 1, Items.ENDER_PEARL);
        if (switchWaterBucket.get()) swapItem(waterBucketSlot.get() - 1, Items.WATER_BUCKET);
        if (switchFireball.get()) swapItem(fireballSlot.get() - 1, Items.FIRE_CHARGE);

        if (keepProjectile.get()) {
            if (InventoryUtils.getItemCount(Items.EGG) + InventoryUtils.getItemCount(Items.SNOWBALL) > maxProjectileSize.get())
                throwItem(InventoryUtils.getWorstProjectile());
            if (switchProjectile.get() && !isOffhand("Projectile")) {
                int target = projectileSlot.get() - 1;
                if (InventoryUtils.getItemCount(Items.EGG) > 0) swapItem(target, Items.EGG);
                else if (InventoryUtils.getItemCount(Items.SNOWBALL) > 0) swapItem(target, Items.SNOWBALL);
            }
        }

        if (throwItems.get()) {
            List<Integer> slots = new ArrayList<>();
            for (int i = 0; i < InventoryUtils.items().size(); i++) slots.add(i);
            Collections.shuffle(slots);
            for (int slot : slots) {
                ItemStack stack = InventoryUtils.item(slot);
                if (!stack.isEmpty() && !isItemUseful(stack)) throwItem(stack);
            }
        }
    }

    /** InventoryMenu armor slots run 5 HEAD .. 8 FEET. */
    private static int armorContainerSlot(EquipmentSlot slot) {
        return switch (slot) {
            case HEAD -> 5;
            case CHEST -> 6;
            case LEGS -> 7;
            default -> 8;
        };
    }

    private void click(int slot, int button, ContainerInput input) {
        mc.gameMode.handleContainerInput(mc.player.inventoryMenu.containerId, slot, button, input, mc.player);
        inventoryOpen = true;
        timer.reset();
    }

    /** Pick the stack up so the next tick's offhand click drops it into slot 45. */
    private void topUpOffHand(int slot) {
        click(InventoryUtils.containerSlot(slot), 0, ContainerInput.PICKUP);
        clickOffHand = true;
    }

    private void swapOffHand(int slot) {
        click(InventoryUtils.containerSlot(slot), 40, ContainerInput.SWAP);
    }

    private void throwItem(ItemStack stack) {
        if (stack == null || !InventoryUtils.isItemValid(stack) || !ready(nextDelay())) return;
        int slot = InventoryUtils.getItemStackSlot(stack);
        if (slot != -1) click(InventoryUtils.containerSlot(slot), 1, ContainerInput.THROW);
    }

    private void swapItem(int target, ItemStack best) {
        ItemStack current = InventoryUtils.item(target);
        if (best == null || !InventoryUtils.isItemValid(current) || best == current || !ready(nextDelay())) return;
        int slot = InventoryUtils.getItemStackSlot(best);
        if (slot != -1) click(InventoryUtils.containerSlot(slot), target, ContainerInput.SWAP);
    }

    private void swapItem(int target, Item item) {
        ItemStack current = InventoryUtils.item(target);
        if (!InventoryUtils.isItemValid(current) || !ready(nextDelay())) return;
        int slot = InventoryUtils.getItemSlot(item);
        if (slot == -1) return;
        ItemStack best = InventoryUtils.item(slot);
        if (current.getItem() != item || current.getCount() < best.getCount())
            click(InventoryUtils.containerSlot(slot), target, ContainerInput.SWAP);
    }

    /** Starain isItemUseful: what this module is willing to keep in the inventory. */
    public boolean isItemUseful(ItemStack stack) {
        if (stack.isEmpty()) return false;
        if (InventoryUtils.isGodItem(stack)) return true;
        if (stack.getDisplayName().getString().contains("点击使用")) return true;
        EquipmentSlot armorSlot = InventoryUtils.armorSlot(stack);
        if (armorSlot != null) {
            float protection = InventoryUtils.getProtection(stack);
            if (InventoryUtils.getCurrentArmorScore(armorSlot) >= protection) return false;
            return protection >= InventoryUtils.getBestArmorScore(armorSlot);
        }
        if (InventoryUtils.isSword(stack)) return InventoryUtils.getBestSword() == stack;
        if (InventoryUtils.isPickaxe(stack)) return InventoryUtils.getBestPickaxe() == stack;
        if (InventoryUtils.isAxe(stack) && !InventoryUtils.isSharpnessAxe(stack))
            return InventoryUtils.getBestAxe() == stack;
        if (InventoryUtils.isShovel(stack)) return InventoryUtils.getBestShovel() == stack;
        if (stack.getItem() instanceof net.minecraft.world.item.CrossbowItem)
            return InventoryUtils.getBestCrossbow() == stack;
        if (stack.getItem() instanceof net.minecraft.world.item.BowItem && InventoryUtils.isPunchBow(stack))
            return InventoryUtils.getBestPunchBow() == stack;
        if (stack.getItem() instanceof net.minecraft.world.item.BowItem && InventoryUtils.isPowerBow(stack))
            return InventoryUtils.getBestPowerBow() == stack;
        if (stack.getItem() instanceof net.minecraft.world.item.BowItem
                && InventoryUtils.getItemCount(Items.BOW) > 1) return false;
        if (stack.getItem() == Items.WATER_BUCKET
                && InventoryUtils.getItemCount(Items.WATER_BUCKET) > getWaterBucketCount()) return false;
        if (stack.getItem() == Items.LAVA_BUCKET
                && InventoryUtils.getItemCount(Items.LAVA_BUCKET) > getLavaBucketCount()) return false;
        if (stack.getItem() instanceof net.minecraft.world.item.FishingRodItem
                && InventoryUtils.getItemCount(Items.FISHING_ROD) > 1) return false;
        if ((stack.getItem() == Items.SNOWBALL || stack.getItem() == Items.EGG) && !shouldKeepProjectile()) return false;
        if (stack.getItem() == Items.GOLDEN_APPLE || stack.getItem() == Items.ENCHANTED_GOLDEN_APPLE) return true;
        return InventoryUtils.isCommonItemUseful(stack);
    }

    /**
     * Starain checkConfig: two features pointing at the same hotbar slot would fight over it every
     * tick. Nothing read its result over there, so this is exposed as the module's info suffix.
     */
    public boolean hasSlotConflict() {
        Set<Integer> used = new HashSet<>();
        List<Setting<Integer>> slots = new ArrayList<>();
        if (switchSword.get()) slots.add(swordSlot);
        if (switchPickaxe.get()) slots.add(pickaxeSlot);
        if (switchAxe.get()) slots.add(axeSlot);
        if (switchBow.get()) slots.add(bowSlot);
        if (switchWaterBucket.get()) slots.add(waterBucketSlot);
        if (switchEnderPearl.get()) slots.add(enderPearlSlot);
        if (switchFireball.get()) slots.add(fireballSlot);
        if (switchGoldenApple.get() && !isOffhand("Golden Apple")) slots.add(goldenAppleSlot);
        if (switchProjectile.get() && keepProjectile.get() && !isOffhand("Projectile")) slots.add(projectileSlot);
        if (switchRod.get() && !isOffhand("Fishing Rod")) slots.add(rodSlot);
        if (switchBlock.get() && !isOffhand("Block")) slots.add(blockSlot);
        for (Setting<Integer> slot : slots) if (!used.add(slot.get())) return true;
        return false;
    }

    @Override public String getInfo() { return hasSlotConflict() ? "slot conflict" : null; }
}
