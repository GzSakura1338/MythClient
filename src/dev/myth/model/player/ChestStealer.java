package dev.myth.model.player;

import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.Priority;
import dev.myth.event.api.annotations.EventPriority;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.MotionEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.TimerUtils;
import dev.myth.utils.inventory.InventoryUtils;
import dev.myth.utils.wrapper.Wrapper;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ThreadLocalRandom;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.gui.screens.inventory.ContainerScreen;
import net.minecraft.network.chat.Component;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.item.BlockItem;
import net.minecraft.world.item.BowItem;
import net.minecraft.world.item.CrossbowItem;
import net.minecraft.world.item.FishingRodItem;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.inventory.ChestMenu;
import net.minecraft.world.inventory.ContainerInput;

/** Starain ChestStealer: quick-moves the items worth taking out of a chest, then closes it. */
public final class ChestStealer extends Model implements Wrapper {
    private static final TimerUtils timer = new TimerUtils();

    public final Setting<Integer> minDelay = addSetting(Setting.integer("minDelay", "Min Delay", 125, 0, 1000).unit("ms"));
    public final Setting<Integer> delay = addSetting(Setting.integer("delay", "Delay", 150, 0, 1000).unit("ms"));
    public final Setting<Boolean> closeDelay = addSetting(Setting.bool("closeDelay", "Close Delay", true).group());
    public final Setting<Integer> closeDelayNumber = addSetting(Setting.integer("closeDelayNumber", "Close Delay Number", 150, 0, 1000)
            .unit("ms").visibleWhen(() -> closeDelay.get() && closeDelay.isExpanded()));
    public final Setting<Boolean> pickEnderChest = addSetting(Setting.bool("pickEnderChest", "Ender Chest", false));

    private Screen lastTickScreen;

    public ChestStealer() {
        super("cheststealer", "ChestStealer", Category.PLAYER);
        setDescription("Takes the useful items out of a chest and closes it.");
    }

    /** Starain isWorking: InvManager stands down while a chest is being emptied. */
    public static boolean isWorking() { return timer.elapsedMillis() < 3; }

    private int nextDelay() {
        return Math.max(minDelay.get(), (int) (delay.get() + ThreadLocalRandom.current().nextGaussian() * 50));
    }

    @EventTarget @EventPriority(Priority.HIGH)
    public void onMotion(MotionEvent event) {
        if (event.getType() != EventType.PRE) return;
        Screen current = mc.gui.screen();
        if (current instanceof ContainerScreen container) {
            ChestMenu menu = container.getMenu();
            if (current != lastTickScreen) {
                timer.reset();
            } else {
                String title = container.getTitle().getString();
                String chest = Component.translatable("container.chest").getString();
                String largeChest = Component.translatable("container.chestDouble").getString();
                String enderChest = Component.translatable("container.enderchest").getString();
                if (title.equals(chest) || title.equals(largeChest) || title.equals("Chest")
                        || (pickEnderChest.get() && title.equals(enderChest))) {
                    int next = nextDelay();
                    if (isChestEmpty(menu) && timer.elapsedMillis() >= next) {
                        if (mc.player != null && closeDelay.get() && timer.elapsedMillis() >= closeDelayNumber.get()) {
                            mc.player.closeContainer();
                            timer.reset();
                        }
                    } else {
                        List<Integer> slots = new ArrayList<>();
                        for (int i = 0; i < menu.getRowCount() * 9; i++) slots.add(i);
                        Collections.shuffle(slots);
                        for (int slot : slots) {
                            ItemStack stack = menu.getSlot(slot).getItem();
                            if (isItemUseful(stack) && isBestItemInChest(menu, stack) && timer.elapsedMillis() >= next) {
                                mc.gameMode.handleContainerInput(menu.containerId, slot, 0, ContainerInput.QUICK_MOVE, mc.player);
                                timer.reset();
                                break;
                            }
                        }
                    }
                }
            }
        }
        lastTickScreen = current;
    }

    private boolean isChestEmpty(ChestMenu menu) {
        for (int i = 0; i < menu.getRowCount() * 9; i++) {
            ItemStack stack = menu.getSlot(i).getItem();
            if (!stack.isEmpty() && isItemUseful(stack) && isBestItemInChest(menu, stack)) return false;
        }
        return true;
    }

    /** Starain isItemUseful: strictly better than what is already carried, and within the caps. */
    public static boolean isItemUseful(ItemStack stack) {
        if (stack.isEmpty()) return false;
        if (InventoryUtils.isGodItem(stack) || InventoryUtils.isSharpnessAxe(stack)) return true;
        EquipmentSlot armorSlot = InventoryUtils.armorSlot(stack);
        if (armorSlot != null)
            return InventoryUtils.getProtection(stack) > InventoryUtils.getBestArmorScore(armorSlot);
        if (InventoryUtils.isSword(stack))
            return InventoryUtils.getSwordDamage(stack) > InventoryUtils.getBestSwordDamage();
        if (InventoryUtils.isPickaxe(stack))
            return InventoryUtils.getToolScore(stack) > InventoryUtils.getBestPickaxeScore();
        if (InventoryUtils.isAxe(stack))
            return InventoryUtils.getToolScore(stack) > InventoryUtils.getBestAxeScore();
        if (InventoryUtils.isShovel(stack))
            return InventoryUtils.getToolScore(stack) > InventoryUtils.getBestShovelScore();
        if (stack.getItem() instanceof CrossbowItem)
            return InventoryUtils.getCrossbowScore(stack) > InventoryUtils.getBestCrossbowScore();
        if (stack.getItem() instanceof BowItem && InventoryUtils.isPunchBow(stack))
            return InventoryUtils.getPunchBowScore(stack) > InventoryUtils.getBestPunchBowScore();
        if (stack.getItem() instanceof BowItem && InventoryUtils.isPowerBow(stack))
            return InventoryUtils.getPowerBowScore(stack) > InventoryUtils.getBestPowerBowScore();
        if (stack.getItem() == Items.COMPASS) return !InventoryUtils.hasItem(stack.getItem());
        if (stack.getItem() == Items.WATER_BUCKET
                && InventoryUtils.getItemCount(Items.WATER_BUCKET) >= InvManager.getWaterBucketCount()) return false;
        if (stack.getItem() == Items.LAVA_BUCKET
                && InventoryUtils.getItemCount(Items.LAVA_BUCKET) >= InvManager.getLavaBucketCount()) return false;
        if (stack.getItem() instanceof BlockItem && InventoryUtils.isValidStack(stack)
                && InventoryUtils.getBlockCountInInventory() + stack.getCount() >= InvManager.getMaxBlockSize()) return false;
        if (stack.getItem() == Items.ARROW
                && InventoryUtils.getItemCount(Items.ARROW) + stack.getCount() >= InvManager.getMaxArrowSize()) return false;
        if (stack.getItem() instanceof FishingRodItem
                && InventoryUtils.getItemCount(Items.FISHING_ROD) >= 1) return false;
        // Starain folds the projectile cap into the common check with an inverted condition.
        boolean projectile = stack.getItem() == Items.SNOWBALL || stack.getItem() == Items.EGG;
        if (projectile && !(InventoryUtils.getItemCount(Items.SNOWBALL) + InventoryUtils.getItemCount(Items.EGG)
                + stack.getCount() < InvManager.getMaxProjectileSize() && InvManager.shouldKeepProjectile())) return false;
        return InventoryUtils.isCommonItemUseful(stack);
    }

    /** Starain isBestItemInChest: do not grab a tool the same chest has a better copy of. */
    private static boolean isBestItemInChest(ChestMenu menu, ItemStack stack) {
        if (InventoryUtils.isGodItem(stack) || InventoryUtils.isSharpnessAxe(stack)) return true;
        EquipmentSlot armorSlot = InventoryUtils.armorSlot(stack);
        for (int i = 0; i < menu.getRowCount() * 9; i++) {
            ItemStack other = menu.getSlot(i).getItem();
            if (armorSlot != null && InventoryUtils.armorSlot(other) == armorSlot) {
                if (InventoryUtils.getProtection(other) > InventoryUtils.getProtection(stack)) return false;
            } else if (InventoryUtils.isSword(stack) && InventoryUtils.isSword(other)) {
                if (InventoryUtils.getSwordDamage(other) > InventoryUtils.getSwordDamage(stack)) return false;
            } else if (InventoryUtils.isPickaxe(stack) && InventoryUtils.isPickaxe(other)) {
                if (InventoryUtils.getToolScore(other) > InventoryUtils.getToolScore(stack)) return false;
            } else if (InventoryUtils.isAxe(stack) && InventoryUtils.isAxe(other)) {
                if (InventoryUtils.getToolScore(other) > InventoryUtils.getToolScore(stack)) return false;
            } else if (InventoryUtils.isShovel(stack) && InventoryUtils.isShovel(other)
                    && InventoryUtils.getToolScore(other) > InventoryUtils.getToolScore(stack)) {
                return false;
            }
        }
        return true;
    }
}
