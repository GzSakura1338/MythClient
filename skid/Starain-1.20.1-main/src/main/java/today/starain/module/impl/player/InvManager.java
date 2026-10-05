package today.starain.module.impl.player;

import net.minecraft.client.gui.screens.inventory.AbstractContainerScreen;
import net.minecraft.client.gui.screens.inventory.InventoryScreen;
import net.minecraft.network.protocol.game.*;
import net.minecraft.world.inventory.ClickType;
import net.minecraft.world.item.*;
import org.apache.commons.lang3.tuple.Pair;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.event.impl.PacketEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.player.InventoryUtils;
import today.starain.utils.player.MoveUtils;
import today.starain.utils.timer.TimerUtils;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;

import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

public class InvManager extends ClientModule {
    private static final TimerUtils timer = new TimerUtils();
    private static final Random random = new Random();

    private final NumberValue minDelay = new NumberValue("Min Delay", 0, 1000, 125, 1, this);
    private final NumberValue delay = new NumberValue("Delay", 0, 1000, 150, 1, this);
    private final ModeValue offhandItems = new ModeValue("Offhand Items", "None", new String[]{"None", "Golden Apple", "Projectile", "Fishing Rod", "Block"}, this);
    private final BooleanValue autoArmor = new BooleanValue("Auto Armor", true, this);
    private final BooleanValue inventoryOnly = new BooleanValue("Inventory Only", true, this);
    private final BooleanValue switchSword = new BooleanValue("Switch Sword", true, this);
    private final NumberValue swordSlot = new NumberValue("Sword Slot", 1.0F, 9.0F, 1.0F, 1.0F, this, () -> this.switchSword.get());
    private final BooleanValue switchBlock = new BooleanValue("Switch Block", true, this, () -> !this.offhandItems.isMode("Block"));
    private final NumberValue blockSlot = new NumberValue("Block Slot", 1.0F, 9.0F, 2.0F, 1.0F, this, () -> this.switchBlock.get() && !this.offhandItems.isMode("Block"));
    private final NumberValue maxBlockSize = new NumberValue("Max Block Size", 64.0F, 512.0F, 256.0F, 64.0F, this, () -> this.switchBlock.get());
    private final BooleanValue switchPickaxe = new BooleanValue("Switch Pickaxe", true, this);
    private final NumberValue pickaxeSlot = new NumberValue("Pickaxe Slot", 1.0F, 9.0F, 3.0F, 1.0F, this, () -> this.switchPickaxe.get());
    private final BooleanValue switchAxe = new BooleanValue("Switch Axe", true, this);
    private final NumberValue axeSlot = new NumberValue("Axe Slot", 1.0F, 9.0F, 4.0F, 1.0F, this, () -> this.switchAxe.get());
    private final BooleanValue switchBow = new BooleanValue("Switch Bow or Crossbow", true, this);
    private final NumberValue bowSlot = new NumberValue("Bow Slot", 1.0F, 9.0F, 5.0F, 1.0F, this, () -> this.switchBow.get());
    private final ModeValue preferBow = new ModeValue("Bow Priority", "Crossbow", new String[]{"Crossbow", "Power Bow", "Punch Bow"}, this, () -> this.switchBow.get());
    private final NumberValue maxArrowSize = new NumberValue("Max Arrow Size", 64.0F, 512.0F, 256.0F, 64.0F, this, () -> this.switchBow.get());
    private final BooleanValue switchWaterBucket = new BooleanValue("Switch Water Bucket", true, this);
    private final NumberValue waterBucketSlot = new NumberValue("Water Bucket Slot", 1.0F, 9.0F, 6.0F, 1.0F, this, () -> this.switchWaterBucket.get());
    private final BooleanValue switchEnderPearl = new BooleanValue("Switch Ender Pearl", true, this);
    private final NumberValue enderPearlSlot = new NumberValue("Ender Pearl Slot", 1.0F, 9.0F, 7.0F, 1.0F, this, () -> this.switchEnderPearl.get());
    private final BooleanValue switchFireball = new BooleanValue("Switch Fireball", true, this);
    private final NumberValue fireballSlot = new NumberValue("Fireball Slot", 1.0F, 9.0F, 8.0F, 1.0F, this, () -> this.switchFireball.get());
    private final BooleanValue switchGoldenApple = new BooleanValue("Switch Golden Apple", true, this, () -> !this.offhandItems.isMode("Golden Apple"));
    private final NumberValue goldenAppleSlot = new NumberValue("Golden Apple Slot", 1.0F, 9.0F, 9.0F, 1.0F, this, () -> this.switchGoldenApple.get() && !this.offhandItems.isMode("Golden Apple"));
    private final BooleanValue throwItems = new BooleanValue("Throw Items", true, this);
    private final NumberValue waterBucketCount = new NumberValue("Keep Water Buckets", 0.0F, 5.0F, 1.0F, 1.0F, this, () -> this.throwItems.get());
    private final NumberValue lavaBucketCount = new NumberValue("Keep Lava Buckets", 0.0F, 5.0F, 1.0F, 1.0F, this, () -> this.throwItems.get());
    private final BooleanValue keepProjectile = new BooleanValue("Keep Eggs & Snowballs", true, this);
    private final BooleanValue switchProjectile = new BooleanValue("Switch Eggs & Snowballs", false, this, () -> this.keepProjectile.get() && !this.offhandItems.isMode("Projectile"));
    private final NumberValue projectileSlot = new NumberValue("Eggs & Snowballs Slot", 1.0F, 9.0F, 9.0F, 1.0F, this, () -> this.switchProjectile.get() && this.keepProjectile.get() && !this.offhandItems.isMode("Projectile"));
    private final NumberValue maxProjectileSize = new NumberValue("Max Eggs & Snowballs Size", 16.0F, 256.0F, 64.0F, 16.0F, this, () -> this.keepProjectile.get());
    private final BooleanValue switchRod = new BooleanValue("Switch Rod", false, this, () -> !this.offhandItems.isMode("Fishing Rod"));
    private final NumberValue rodSlot = new NumberValue("Rod Slot", 1.0F, 9.0F, 9.0F, 1.0F, this, () -> this.switchRod.get() && !this.offhandItems.isMode("Fishing Rod"));

    int noMoveTicks = 0;
    private boolean clickOffHand = false;
    private boolean inventoryOpen = false;

    public InvManager() {
        setName("InvManager");
        setCategory(Category.PLAYER);
    }

    public static int getMaxBlockSize() {
        return Starain.getInstance().getModuleManager().getModule(InvManager.class).maxBlockSize.get().intValue();
    }

    public static boolean shouldKeepProjectile() {
        return Starain.getInstance().getModuleManager().getModule(InvManager.class).keepProjectile.get();
    }

    public static int getMaxProjectileSize() {
        return Starain.getInstance().getModuleManager().getModule(InvManager.class).maxProjectileSize.get().intValue();
    }

    public static int getMaxArrowSize() {
        return Starain.getInstance().getModuleManager().getModule(InvManager.class).maxArrowSize.get().intValue();
    }

    public static int getWaterBucketCount() {
        return Starain.getInstance().getModuleManager().getModule(InvManager.class).waterBucketCount.get().intValue();
    }

    public static int getLavaBucketCount() {
        return Starain.getInstance().getModuleManager().getModule(InvManager.class).lavaBucketCount.get().intValue();
    }

    public boolean isItemUseful(ItemStack stack) {
        if (stack.isEmpty()) return false;
        if (InventoryUtils.isGodItem(stack)) return true;
        if (stack.getDisplayName().getString().contains("点击使用")) return true;
        if (stack.getItem() instanceof ArmorItem item) {
            float protection = InventoryUtils.getProtection(stack);
            if (InventoryUtils.getCurrentArmorScore(item.getEquipmentSlot()) >= protection) return false;
            float bestArmor = InventoryUtils.getBestArmorScore(item.getEquipmentSlot());
            return !(protection < bestArmor);
        }
        if (stack.getItem() instanceof SwordItem) return InventoryUtils.getBestSword() == stack;
        if (stack.getItem() instanceof PickaxeItem) return InventoryUtils.getBestPickaxe() == stack;
        if (stack.getItem() instanceof AxeItem && !InventoryUtils.isSharpnessAxe(stack))
            return InventoryUtils.getBestAxe() == stack;
        if (stack.getItem() instanceof ShovelItem) return InventoryUtils.getBestShovel() == stack;
        if (stack.getItem() instanceof CrossbowItem) return InventoryUtils.getBestCrossbow() == stack;
        if (stack.getItem() instanceof BowItem && InventoryUtils.isPunchBow(stack))
            return InventoryUtils.getBestPunchBow() == stack;
        if (stack.getItem() instanceof BowItem && InventoryUtils.isPowerBow(stack))
            return InventoryUtils.getBestPowerBow() == stack;
        if (stack.getItem() instanceof BowItem && InventoryUtils.getItemCount(Items.BOW) > 1) return false;
        if (stack.getItem() == Items.WATER_BUCKET && InventoryUtils.getItemCount(Items.WATER_BUCKET) > getWaterBucketCount())
            return false;
        if (stack.getItem() == Items.LAVA_BUCKET && InventoryUtils.getItemCount(Items.LAVA_BUCKET) > getLavaBucketCount())
            return false;
        if (stack.getItem() instanceof FishingRodItem && InventoryUtils.getItemCount(Items.FISHING_ROD) > 1)
            return false;
        if ((stack.getItem() == Items.SNOWBALL || stack.getItem() == Items.EGG) && !shouldKeepProjectile())
            return false;
        if (stack.getItem() == Items.GOLDEN_APPLE || stack.getItem() == Items.ENCHANTED_GOLDEN_APPLE) return true;
        return !(stack.getItem() instanceof ItemNameBlockItem) && InventoryUtils.isCommonItemUseful(stack);
    }

    @EventTarget
    public void onPacket(PacketEvent e) {
        if (e.getType() == EventType.OUTGOING) {
            if (e.getPacket() instanceof ServerboundContainerClosePacket) this.inventoryOpen = false;
            if (this.inventoryOpen && !this.inventoryOnly.get()) {
                if (e.getPacket() instanceof ServerboundMovePlayerPacket) {
                    if (MoveUtils.isMoving())
                        mc.getConnection().send(new ServerboundContainerClosePacket(mc.player.inventoryMenu.containerId));
                } else if (e.getPacket() instanceof ServerboundUseItemOnPacket || e.getPacket() instanceof ServerboundUseItemPacket || e.getPacket() instanceof ServerboundInteractPacket || e.getPacket() instanceof ServerboundPlayerActionPacket) {
                    mc.getConnection().send(new ServerboundContainerClosePacket(mc.player.inventoryMenu.containerId));
                }
            }
        }
    }

    private boolean checkConfig() {
        List<Pair<BooleanValue, NumberValue>> pairs = new ArrayList<>();
        if (!this.keepProjectile.get()) this.switchProjectile.set(false);
        pairs.add(Pair.of(this.switchSword, this.swordSlot));
        pairs.add(Pair.of(this.switchPickaxe, this.pickaxeSlot));
        pairs.add(Pair.of(this.switchAxe, this.axeSlot));
        pairs.add(Pair.of(this.switchBow, this.bowSlot));
        pairs.add(Pair.of(this.switchWaterBucket, this.waterBucketSlot));
        pairs.add(Pair.of(this.switchEnderPearl, this.enderPearlSlot));
        pairs.add(Pair.of(this.switchFireball, this.fireballSlot));
        if (!this.offhandItems.isMode("Golden Apple")) pairs.add(Pair.of(this.switchGoldenApple, this.goldenAppleSlot));
        if (!this.offhandItems.isMode("Projectile")) pairs.add(Pair.of(this.switchProjectile, this.projectileSlot));
        if (!this.offhandItems.isMode("Fishing Rod")) pairs.add(Pair.of(this.switchRod, this.rodSlot));
        if (!this.offhandItems.isMode("Block")) pairs.add(Pair.of(this.switchBlock, this.blockSlot));
        Set<Integer> usedSlot = new HashSet<>();
        for (Pair<BooleanValue, NumberValue> pair : pairs) {
            if (pair.getKey().get()) {
                int targetSlot = pair.getValue().get().intValue() - 1;
                if (usedSlot.contains(targetSlot)) return false;
                usedSlot.add(targetSlot);
            }
        }
        return true;
    }

    @EventTarget
    public void onMotion(MotionEvent e) {
        if (e.getType() == EventType.PRE) {
            if (InventoryUtils.shouldDisableFeatures()) return;
            if (MoveUtils.isMoving()) this.noMoveTicks = 0;
            else this.noMoveTicks++;
            boolean allowMove = !this.inventoryOnly.get();
            if (ChestStealer.isWorking() || (this.inventoryOnly.get() ? !(mc.screen instanceof InventoryScreen) : (!allowMove && this.noMoveTicks <= 1))) {
                this.clickOffHand = false;
                return;
            }
            if (mc.screen instanceof AbstractContainerScreen container && container.getMenu().containerId != mc.player.inventoryMenu.containerId)
                return;
            int nextDelay = Math.max(minDelay.get().intValue(), (int) (this.delay.get().intValue() + random.nextGaussian() * 50));

            if (this.autoArmor.get()) {
                for (int i = 0; i < mc.player.getInventory().armor.size(); i++) {
                    ItemStack stack = mc.player.getInventory().armor.get(i);
                    if (stack.getItem() instanceof ArmorItem item) {
                        if (!stack.isEmpty() && timer.delay(nextDelay) && InventoryUtils.getBestArmorScore(item.getEquipmentSlot()) > InventoryUtils.getProtection(stack)) {
                            mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, 4 + (4 - i), 1, ClickType.THROW, mc.player);
                            this.inventoryOpen = true;
                            timer.reset();
                        }
                    }
                }
                for (int ix = 0; ix < mc.player.getInventory().items.size(); ix++) {
                    ItemStack stack = mc.player.getInventory().items.get(ix);
                    if (!stack.isEmpty() && stack.getItem() instanceof ArmorItem item) {
                        float currentItemScore = InventoryUtils.getProtection(stack);
                        boolean isBestItem = InventoryUtils.getBestArmorScore(item.getEquipmentSlot()) == currentItemScore;
                        boolean isBetterItem = InventoryUtils.getCurrentArmorScore(item.getEquipmentSlot()) < currentItemScore;
                        if (isBestItem && isBetterItem && timer.delay(nextDelay)) {
                            if (ix < 9)
                                mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, ix + 36, 0, ClickType.QUICK_MOVE, mc.player);
                            else
                                mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, ix, 0, ClickType.QUICK_MOVE, mc.player);
                            this.inventoryOpen = true;
                            timer.reset();
                        }
                    }
                }
            }

            if (this.clickOffHand && timer.delay(nextDelay)) {
                mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, 45, 0, ClickType.PICKUP, mc.player);
                this.inventoryOpen = true;
                this.clickOffHand = false;
                timer.reset();
            }

            if (this.offhandItems.isMode("Golden Apple")) {
                ItemStack offHand = mc.player.getInventory().offhand.get(0);
                Item offHandItem = offHand.getItem();

                int egapSlot = InventoryUtils.getItemSlot(Items.ENCHANTED_GOLDEN_APPLE);
                int gapSlot = InventoryUtils.getItemSlot(Items.GOLDEN_APPLE);

                if (offHandItem == Items.ENCHANTED_GOLDEN_APPLE) {
                    if (offHand.getCount() < offHand.getMaxStackSize() && egapSlot != -1) {
                        if (timer.delay(nextDelay)) {
                            int targetSlot = egapSlot;
                            if (targetSlot < 9)
                                mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, targetSlot + 36, 0, ClickType.PICKUP, mc.player);
                            else
                                mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, targetSlot, 0, ClickType.PICKUP, mc.player);
                            this.inventoryOpen = true;
                            this.clickOffHand = true;
                            timer.reset();
                        }
                    }
                } else if (offHandItem == Items.GOLDEN_APPLE) {
                    if (egapSlot != -1) {
                        if (timer.delay(nextDelay)) {
                            this.swapOffHand(egapSlot);
                            timer.reset();
                        }
                    } else if (offHand.getCount() < offHand.getMaxStackSize() && gapSlot != -1) {
                        if (timer.delay(nextDelay)) {
                            int targetSlot = gapSlot;
                            if (targetSlot < 9)
                                mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, targetSlot + 36, 0, ClickType.PICKUP, mc.player);
                            else
                                mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, targetSlot, 0, ClickType.PICKUP, mc.player);
                            this.inventoryOpen = true;
                            this.clickOffHand = true;
                            timer.reset();
                        }
                    }
                } else {
                    if (egapSlot != -1) {
                        if (timer.delay(nextDelay)) {
                            this.swapOffHand(egapSlot);
                            timer.reset();
                        }
                    } else if (gapSlot != -1) {
                        if (timer.delay(nextDelay)) {
                            this.swapOffHand(gapSlot);
                            timer.reset();
                        }
                    }
                }
            } else if (this.offhandItems.isMode("Projectile")) {
                ItemStack offHand = mc.player.getInventory().offhand.get(0);
                ItemStack bestProjectile = InventoryUtils.getBestProjectile();
                if (bestProjectile != null) {
                    int slot = InventoryUtils.getItemStackSlot(bestProjectile);
                    boolean shouldSwap = offHand.getItem() != Items.EGG && offHand.getItem() != Items.SNOWBALL || offHand.getCount() < bestProjectile.getCount();
                    if (shouldSwap && slot != -1 && timer.delay(nextDelay)) this.swapOffHand(slot);
                }
            } else if (this.offhandItems.isMode("Fishing Rod")) {
                ItemStack offHand = mc.player.getInventory().offhand.get(0);
                int slotx = InventoryUtils.getItemSlot(Items.FISHING_ROD);
                if (slotx != -1 && timer.delay(nextDelay) && offHand.getItem() != Items.FISHING_ROD)
                    this.swapOffHand(slotx);
            } else if (this.offhandItems.isMode("Block")) {
                ItemStack offHand = mc.player.getInventory().offhand.get(0);
                ItemStack bestBlock = InventoryUtils.getBestBlock();
                if (bestBlock != null) {
                    int slotx = InventoryUtils.getItemStackSlot(bestBlock);
                    boolean shouldSwapx = !InventoryUtils.isValidStack(offHand) || offHand.getCount() < bestBlock.getCount();
                    if (shouldSwapx && slotx != -1 && timer.delay(nextDelay)) this.swapOffHand(slotx);
                }
            }

            if (this.switchGoldenApple.get() && !this.offhandItems.isMode("Golden Apple")) {
                int targetSlotIdx = this.goldenAppleSlot.get().intValue() - 1;
                int egapSlot = InventoryUtils.getItemSlot(Items.ENCHANTED_GOLDEN_APPLE);
                int gapSlot = InventoryUtils.getItemSlot(Items.GOLDEN_APPLE);
                int bestGapSlot = (egapSlot != -1) ? egapSlot : gapSlot;
                if (bestGapSlot != -1) {
                    ItemStack currentInSlot = mc.player.getInventory().items.get(targetSlotIdx);
                    ItemStack bestGapItem = mc.player.getInventory().items.get(bestGapSlot);
                    if (currentInSlot.getItem() != bestGapItem.getItem() || (currentInSlot.getCount() < bestGapItem.getCount() && currentInSlot.getItem() == bestGapItem.getItem())) {
                        this.swapItem(targetSlotIdx, bestGapItem);
                    }
                }
            }

            if (this.switchBlock.get()) {
                int blockSlotIndex = this.blockSlot.get().intValue() - 1;
                ItemStack currentBlock = mc.player.getInventory().items.get(blockSlotIndex);
                ItemStack bestBlock = InventoryUtils.getBestBlock();
                if (bestBlock != null && (bestBlock.getCount() > currentBlock.getCount() || !InventoryUtils.isValidStack(currentBlock)) && !this.offhandItems.isMode("Block")) {
                    this.swapItem(blockSlotIndex, bestBlock);
                }
                if ((float) InventoryUtils.getBlockCountInInventory() > this.maxBlockSize.get().intValue()) {
                    this.throwItem(InventoryUtils.getWorstBlock());
                }
            }

            if (this.switchSword.get()) {
                int slotIndex = this.swordSlot.get().intValue() - 1;
                ItemStack currentSword = mc.player.getInventory().items.get(slotIndex);
                ItemStack bestSword = InventoryUtils.getBestSword();
                ItemStack bestShapeAxe = InventoryUtils.getBestShapeAxe();
                if (InventoryUtils.getAxeDamage(bestShapeAxe) > InventoryUtils.getSwordDamage(bestSword))
                    bestSword = bestShapeAxe;
                if (bestSword != null) {
                    float currentDamage = currentSword.getItem() instanceof SwordItem ? InventoryUtils.getSwordDamage(currentSword) : InventoryUtils.getAxeDamage(currentSword);
                    float bestWeaponDamage = bestSword.getItem() instanceof SwordItem ? InventoryUtils.getSwordDamage(bestSword) : InventoryUtils.getAxeDamage(bestSword);
                    if (bestWeaponDamage > currentDamage) this.swapItem(slotIndex, bestSword);
                }
            }

            if (this.switchPickaxe.get()) {
                int slotIndex = this.pickaxeSlot.get().intValue() - 1;
                ItemStack bestPickaxe = InventoryUtils.getBestPickaxe();
                ItemStack currentPickaxe = mc.player.getInventory().items.get(slotIndex);
                if (bestPickaxe != null && bestPickaxe.getItem() instanceof PickaxeItem && (InventoryUtils.getToolScore(bestPickaxe) > InventoryUtils.getToolScore(currentPickaxe) || !(currentPickaxe.getItem() instanceof PickaxeItem)))
                    this.swapItem(slotIndex, bestPickaxe);
            }

            if (this.switchAxe.get()) {
                int slotIndex = this.axeSlot.get().intValue() - 1;
                ItemStack bestAxe = InventoryUtils.getBestAxe();
                ItemStack currentAxe = mc.player.getInventory().items.get(slotIndex);
                if (bestAxe != null && bestAxe.getItem() instanceof AxeItem && (InventoryUtils.getToolScore(bestAxe) > InventoryUtils.getToolScore(currentAxe) || !(currentAxe.getItem() instanceof AxeItem)))
                    this.swapItem(slotIndex, bestAxe);
            }

            if (this.switchRod.get() && !this.offhandItems.isMode("Fishing Rod")) {
                int slotIndex = this.rodSlot.get().intValue() - 1;
                ItemStack bestRod = InventoryUtils.getFishingRod();
                ItemStack currentRod = mc.player.getInventory().items.get(slotIndex);
                if (!(currentRod.getItem() instanceof FishingRodItem)) this.swapItem(slotIndex, bestRod);
            }

            if (this.switchBow.get()) {
                int slotIndex = this.bowSlot.get().intValue() - 1;
                ItemStack currentBow = mc.player.getInventory().items.get(slotIndex);
                ItemStack bestBow;
                float bestScore, currentScore;
                if (this.preferBow.isMode("Crossbow")) {
                    bestBow = InventoryUtils.getBestCrossbow();
                    bestScore = InventoryUtils.getCrossbowScore(bestBow);
                    currentScore = InventoryUtils.getCrossbowScore(currentBow);
                } else if (this.preferBow.isMode("Power Bow")) {
                    bestBow = InventoryUtils.getBestPowerBow();
                    bestScore = InventoryUtils.getPowerBowScore(bestBow);
                    currentScore = InventoryUtils.getPowerBowScore(currentBow);
                } else {
                    bestBow = InventoryUtils.getBestPunchBow();
                    bestScore = InventoryUtils.getPunchBowScore(bestBow);
                    currentScore = InventoryUtils.getPunchBowScore(currentBow);
                }
                if (bestBow == null) {
                    bestBow = InventoryUtils.getBestCrossbow();
                    bestScore = InventoryUtils.getCrossbowScore(bestBow);
                    currentScore = InventoryUtils.getCrossbowScore(currentBow);
                }
                if (bestBow == null) {
                    bestBow = InventoryUtils.getBestPowerBow();
                    bestScore = InventoryUtils.getPowerBowScore(bestBow);
                    currentScore = InventoryUtils.getPowerBowScore(currentBow);
                }
                if (bestBow == null) {
                    bestBow = InventoryUtils.getBestPunchBow();
                    bestScore = InventoryUtils.getPunchBowScore(bestBow);
                    currentScore = InventoryUtils.getPunchBowScore(currentBow);
                }
                if (bestBow != null && bestScore > currentScore) this.swapItem(slotIndex, bestBow);
                if ((float) InventoryUtils.getItemCount(Items.ARROW) > this.maxArrowSize.get().intValue())
                    this.throwItem(InventoryUtils.getWorstArrow());
            }

            if (this.switchEnderPearl.get())
                this.swapItem(this.enderPearlSlot.get().intValue() - 1, Items.ENDER_PEARL);
            if (this.switchWaterBucket.get())
                this.swapItem(this.waterBucketSlot.get().intValue() - 1, Items.WATER_BUCKET);
            if (this.switchFireball.get())
                this.swapItem(this.fireballSlot.get().intValue() - 1, Items.FIRE_CHARGE);

            if (this.keepProjectile.get()) {
                if ((float) (InventoryUtils.getItemCount(Items.EGG) + InventoryUtils.getItemCount(Items.SNOWBALL)) > this.maxProjectileSize.get().intValue())
                    this.throwItem(InventoryUtils.getWorstProjectile());
                if (this.switchProjectile.get() && !this.offhandItems.isMode("Projectile")) {
                    int pSlot = this.projectileSlot.get().intValue() - 1;
                    if (InventoryUtils.getItemCount(Items.EGG) > 0) this.swapItem(pSlot, Items.EGG);
                    else if (InventoryUtils.getItemCount(Items.SNOWBALL) > 0) this.swapItem(pSlot, Items.SNOWBALL);
                }
            }

            if (this.throwItems.get()) {
                List<Integer> slots = IntStream.range(0, mc.player.getInventory().items.size()).boxed().collect(Collectors.toList());
                Collections.shuffle(slots);
                for (Integer slotIdx : slots) {
                    ItemStack stack = mc.player.getInventory().items.get(slotIdx);
                    if (!stack.isEmpty() && !this.isItemUseful(stack)) this.throwItem(stack);
                }
            }
        }
    }

    private void swapOffHand(int slot) {
        if (slot < 9)
            mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, slot + 36, 40, ClickType.SWAP, mc.player);
        else
            mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, slot, 40, ClickType.SWAP, mc.player);
        this.inventoryOpen = true;
        timer.reset();
    }

    private void throwItem(ItemStack item) {
        int nextDelay = Math.max(minDelay.get().intValue(), (int) (this.delay.get().intValue() + random.nextGaussian() * 50));
        if (InventoryUtils.isItemValid(item) && timer.delay(nextDelay)) {
            int itemSlot = InventoryUtils.getItemStackSlot(item);
            if (itemSlot != -1) {
                if (itemSlot < 9)
                    mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, itemSlot + 36, 1, ClickType.THROW, mc.player);
                else
                    mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, itemSlot, 1, ClickType.THROW, mc.player);
                this.inventoryOpen = true;
                timer.reset();
            }
        }
    }

    private void swapItem(int targetSlot, ItemStack bestItem) {
        ItemStack currentSlot = mc.player.getInventory().items.get(targetSlot);
        int nextDelay = Math.max(minDelay.get().intValue(), (int) (this.delay.get().intValue() + random.nextGaussian() * 50));
        if (InventoryUtils.isItemValid(currentSlot) && bestItem != currentSlot && timer.delay(nextDelay)) {
            int bestItemSlot = InventoryUtils.getItemStackSlot(bestItem);
            if (bestItemSlot != -1) {
                if (bestItemSlot < 9)
                    mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, bestItemSlot + 36, targetSlot, ClickType.SWAP, mc.player);
                else
                    mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, bestItemSlot, targetSlot, ClickType.SWAP, mc.player);
                this.inventoryOpen = true;
                timer.reset();
            }
        }
    }

    private void swapItem(int targetSlot, Item item) {
        ItemStack currentSlot = mc.player.getInventory().items.get(targetSlot);
        int nextDelay = Math.max(minDelay.get().intValue(), (int) (this.delay.get().intValue() + random.nextGaussian() * 50));
        if (InventoryUtils.isItemValid(currentSlot) && timer.delay(nextDelay)) {
            int bestItemSlot = InventoryUtils.getItemSlot(item);
            if (bestItemSlot != -1) {
                ItemStack bestItemStack = mc.player.getInventory().items.get(bestItemSlot);
                if (currentSlot.getItem() != item || currentSlot.getItem() == item && currentSlot.getCount() < bestItemStack.getCount()) {
                    if (bestItemSlot < 9)
                        mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, bestItemSlot + 36, targetSlot, ClickType.SWAP, mc.player);
                    else
                        mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId, bestItemSlot, targetSlot, ClickType.SWAP, mc.player);
                    this.inventoryOpen = true;
                    timer.reset();
                }
            }
        }
    }
}
