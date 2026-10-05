package today.starain.module.impl.player;

import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.gui.screens.inventory.ContainerScreen;
import net.minecraft.network.chat.Component;
import net.minecraft.world.inventory.ChestMenu;
import net.minecraft.world.inventory.ClickType;
import net.minecraft.world.item.*;
import today.starain.event.api.EventType;
import today.starain.event.api.Priority;
import today.starain.event.api.annotations.EventPriority;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.player.InventoryUtils;
import today.starain.utils.timer.TimerUtils;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.NumberValue;

import java.util.Collections;
import java.util.List;
import java.util.Random;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

public class ChestStealer extends ClientModule {
    private static final TimerUtils timer = new TimerUtils();
    private static final Random random = new Random();
    private final NumberValue minDelay = new NumberValue("Min Delay", 0, 1000, 125, 1, this);
    private final NumberValue delay = new NumberValue("Delay", 0, 1000, 150, 1, this);
    private final BooleanValue closeDelay = new BooleanValue("Close Delay", true, this);
    private final NumberValue cDelay = new NumberValue("Close Delay Number", 0, 1000, 150, 1, this, closeDelay::get);
    private final BooleanValue pickEnderChest = new BooleanValue("Ender Chest", false, this);
    private Screen lastTickScreen;

    public ChestStealer() {
        setName("ChestStealer");
        setCategory(Category.PLAYER);
    }

    public static boolean isWorking() {
        return !timer.delay(3);
    }

    public static boolean isItemUseful(ItemStack stack) {
        if (stack.isEmpty()) {
            return false;
        } else if (InventoryUtils.isGodItem(stack) || InventoryUtils.isSharpnessAxe(stack)) {
            return true;
        } else if (stack.getItem() instanceof ArmorItem item) {
            float protection = InventoryUtils.getProtection(stack);
            float bestArmor = InventoryUtils.getBestArmorScore(item.getEquipmentSlot());
            return !(protection <= bestArmor);
        } else if (stack.getItem() instanceof SwordItem) {
            float damage = InventoryUtils.getSwordDamage(stack);
            float bestDamage = InventoryUtils.getBestSwordDamage();
            return !(damage <= bestDamage);
        } else if (stack.getItem() instanceof PickaxeItem) {
            float score = InventoryUtils.getToolScore(stack);
            float bestScore = InventoryUtils.getBestPickaxeScore();
            return !(score <= bestScore);
        } else if (stack.getItem() instanceof AxeItem) {
            float score = InventoryUtils.getToolScore(stack);
            float bestScore = InventoryUtils.getBestAxeScore();
            return !(score <= bestScore);
        } else if (stack.getItem() instanceof ShovelItem) {
            float score = InventoryUtils.getToolScore(stack);
            float bestScore = InventoryUtils.getBestShovelScore();
            return !(score <= bestScore);
        } else if (stack.getItem() instanceof CrossbowItem) {
            float score = InventoryUtils.getCrossbowScore(stack);
            float bestScore = InventoryUtils.getBestCrossbowScore();
            return !(score <= bestScore);
        } else if (stack.getItem() instanceof BowItem && InventoryUtils.isPunchBow(stack)) {
            float score = InventoryUtils.getPunchBowScore(stack);
            float bestScore = InventoryUtils.getBestPunchBowScore();
            return !(score <= bestScore);
        } else if (stack.getItem() instanceof BowItem && InventoryUtils.isPowerBow(stack)) {
            float score = InventoryUtils.getPowerBowScore(stack);
            float bestScore = InventoryUtils.getBestPowerBowScore();
            return !(score <= bestScore);
        } else if (stack.getItem() == Items.COMPASS) {
            return !InventoryUtils.hasItem(stack.getItem());
        } else if (stack.getItem() == Items.WATER_BUCKET && InventoryUtils.getItemCount(Items.WATER_BUCKET) >= InvManager.getWaterBucketCount()) {
            return false;
        } else if (stack.getItem() == Items.LAVA_BUCKET && InventoryUtils.getItemCount(Items.LAVA_BUCKET) >= InvManager.getLavaBucketCount()) {
            return false;
        } else if (stack.getItem() instanceof BlockItem
                && InventoryUtils.isValidStack(stack)
                && InventoryUtils.getBlockCountInInventory() + stack.getCount() >= InvManager.getMaxBlockSize()) {
            return false;
        } else if (stack.getItem() == Items.ARROW && InventoryUtils.getItemCount(Items.ARROW) + stack.getCount() >= InvManager.getMaxArrowSize()) {
            return false;
        } else if (stack.getItem() instanceof FishingRodItem && InventoryUtils.getItemCount(Items.FISHING_ROD) >= 1) {
            return false;
        } else if (stack.getItem() != Items.SNOWBALL && stack.getItem() != Items.EGG
                || InventoryUtils.getItemCount(Items.SNOWBALL) + InventoryUtils.getItemCount(Items.EGG) + stack.getCount() < InvManager.getMaxProjectileSize()
                && InvManager.shouldKeepProjectile()) {
            return !(stack.getItem() instanceof ItemNameBlockItem) && InventoryUtils.isCommonItemUseful(stack);
        } else {
            return false;
        }
    }

    private static boolean isBestItemInChest(ChestMenu menu, ItemStack stack) {
        if (!InventoryUtils.isGodItem(stack) && !InventoryUtils.isSharpnessAxe(stack)) {
            for (int i = 0; i < menu.getRowCount() * 9; i++) {
                ItemStack checkStack = menu.getSlot(i).getItem();
                if (stack.getItem() instanceof ArmorItem item && checkStack.getItem() instanceof ArmorItem checkItem) {
                    if (item.getEquipmentSlot() == checkItem.getEquipmentSlot() && InventoryUtils.getProtection(checkStack) > InventoryUtils.getProtection(stack)) {
                        return false;
                    }
                } else if (stack.getItem() instanceof SwordItem && checkStack.getItem() instanceof SwordItem) {
                    if (InventoryUtils.getSwordDamage(checkStack) > InventoryUtils.getSwordDamage(stack)) {
                        return false;
                    }
                } else if (stack.getItem() instanceof PickaxeItem && checkStack.getItem() instanceof PickaxeItem) {
                    if (InventoryUtils.getToolScore(checkStack) > InventoryUtils.getToolScore(stack)) {
                        return false;
                    }
                } else if (stack.getItem() instanceof AxeItem && checkStack.getItem() instanceof AxeItem) {
                    if (InventoryUtils.getToolScore(checkStack) > InventoryUtils.getToolScore(stack)) {
                        return false;
                    }
                } else if (stack.getItem() instanceof ShovelItem
                        && checkStack.getItem() instanceof ShovelItem
                        && InventoryUtils.getToolScore(checkStack) > InventoryUtils.getToolScore(stack)) {
                    return false;
                }
            }

            return true;
        } else {
            return true;
        }
    }

    @EventPriority(Priority.HIGH)
    @EventTarget
    public void onMotion(MotionEvent e) {
        if (e.getType() == EventType.PRE) {
            Screen currentScreen = mc.screen;
            if (currentScreen instanceof ContainerScreen container) {
                ChestMenu menu = container.getMenu();
                if (currentScreen != this.lastTickScreen) {
                    timer.reset();
                } else {
                    String chestTitle = container.getTitle().getString();
                    String chest = Component.translatable("container.chest").getString();
                    String largeChest = Component.translatable("container.chestDouble").getString();
                    String enderChest = Component.translatable("container.enderchest").getString();
                    if (chestTitle.equals(chest)
                            || chestTitle.equals(largeChest)
                            || chestTitle.equals("Chest")
                            || this.pickEnderChest.get() && chestTitle.equals(enderChest)) {
                        int nextDelay = Math.max(minDelay.get().intValue(), (int) (this.delay.get().intValue() + random.nextGaussian() * 50));
                        if (this.isChestEmpty(menu) && timer.delay(nextDelay)) {
                            if (mc.player != null && closeDelay.get() && timer.delay(cDelay.get().longValue())) {
                                mc.player.closeContainer();
                                timer.reset();
                            }

                        } else {
                            List<Integer> slots = IntStream.range(0, menu.getRowCount() * 9).boxed().collect(Collectors.toList());
                            Collections.shuffle(slots);

                            for (Integer pSlotId : slots) {
                                ItemStack stack = menu.getSlot(pSlotId).getItem();
                                if (isItemUseful(stack) && isBestItemInChest(menu, stack) && timer.delay(nextDelay)) {
                                    mc.gameMode.handleInventoryMouseClick(menu.containerId, pSlotId, 0, ClickType.QUICK_MOVE, mc.player);
                                    timer.reset();
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            this.lastTickScreen = currentScreen;
        }
    }

    private boolean isChestEmpty(ChestMenu menu) {
        for (int i = 0; i < menu.getRowCount() * 9; i++) {
            ItemStack item = menu.getSlot(i).getItem();
            if (!item.isEmpty() && isItemUseful(item) && isBestItemInChest(menu, item)) {
                return false;
            }
        }

        return true;
    }
}
