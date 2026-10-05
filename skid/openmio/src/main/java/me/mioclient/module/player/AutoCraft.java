package me.mioclient.module.player;

import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.Helper_7;
import me.mioclient.HoleSnapData_2;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.SearchHelper4_7;
import me.mioclient.SearchHelper4_8;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.SetScreenHookPreEvent;
import me.mioclient.event.TickPostEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.block.Blocks;
import net.minecraft.client.gui.screen.ingame.CraftingScreen;
import net.minecraft.client.gui.screen.ingame.HandledScreen;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.component.type.FireworksComponent;
import net.minecraft.item.Item;
import net.minecraft.item.ItemConvertible;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.item.SmithingTemplateItem;
import net.minecraft.network.packet.c2s.play.CloseHandledScreenC2SPacket;
import net.minecraft.recipe.Ingredient;
import net.minecraft.recipe.Recipe;
import net.minecraft.recipe.RecipeEntry;
import net.minecraft.recipe.RecipeType;
import net.minecraft.screen.CraftingScreenHandler;
import net.minecraft.screen.slot.Slot;
import net.minecraft.screen.slot.SlotActionType;
import net.minecraft.util.Hand;
import net.minecraft.util.Identifier;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;
import net.minecraft.util.math.MathHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoCraft.class */
public class AutoCraft extends Module {
    public static final RecipeEntry<?> recipeEntry = new RecipeEntry<>(Identifier.of("minecraft:firework_rocket_simple"), (Recipe) null);
    public static final List<Ingredient> list = List.of(Ingredient.ofItems(new ItemConvertible[]{Items.PAPER}), Ingredient.ofItems(new ItemConvertible[]{Items.GUNPOWDER}));
    public Setting<Set<Item>> items;
    public Setting<Set<Item>> recipeBlackList;
    public Setting<Integer> delay;
    public Setting<Integer> frequency;
    public Setting<AutoCraftMode> drop;
    public Setting<Boolean> craftAll;
    public Setting<Boolean> fastClose;
    public Setting<Boolean> autoOpen;
    public Setting<Float> range;
    public Setting<Boolean> raytrace;
    public Setting<Boolean> strictDirection;
    public Setting<Boolean> rotate;
    public Setting<Boolean> limit;
    public Setting<Integer> max;
    public Setting<Boolean> stackLimit;
    public final Stopwatch stopwatch;
    public final Stopwatch stopwatch2;

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoCraft$AutoCraftMode.class */
    public enum AutoCraftMode implements EnumSettingHelper {
        None("None", SlotActionType.QUICK_MOVE),
        Throw("Throw", SlotActionType.THROW),
        Pickup("Pickup", SlotActionType.PICKUP);

        public final String name;
        public final SlotActionType slotActionType;

        AutoCraftMode(String str, SlotActionType slotActionType) {
            this.name = str;
            this.slotActionType = slotActionType;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public SlotActionType getSlotActionType2107() {
            return this.slotActionType;
        }
    }

    public AutoCraft() {
        super("AutoCraft", "Automatically crafts listed items.", Category.PLAYER, "craftbot");
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
        this.stopwatch2 = new Stopwatch();
    }

    @Listen
    public void onTickPost(TickPostEvent tickPostEvent) {
        HoleSnapData_2<BlockPos, Direction> holeSnapData_2838;
        RecipeEntry<?> recipeEntry834 = getRecipeEntry834();
        if (recipeEntry834 == null) {
            return;
        }
        if (this.autoOpen.getValue().booleanValue() && this.stopwatch2.is419(500L) && !(minecraftClient.currentScreen instanceof HandledScreen) && (holeSnapData_2838 = getHoleSnapData_2838()) != null) {
            if (this.rotate.getValue().booleanValue()) {
                AutoSignSearchHelper4.do2561(SearchHelper4_8.getFloatArray2484(holeSnapData_2838.getObject3119().toCenterPos()), minecraftClient.player.isOnGround());
            }
            AutoSignSearchHelper4.do2556(Hand.MAIN_HAND, new BlockHitResult(holeSnapData_2838.getObject3119().toCenterPos(), holeSnapData_2838.getObject3120(), holeSnapData_2838.getObject3119(), false));
            AutoSignSearchHelper4.do2559(Hand.MAIN_HAND);
            this.stopwatch2.reset();
        }
        if ((minecraftClient.player.currentScreenHandler instanceof CraftingScreenHandler) && !this.items.getValue().isEmpty() && this.stopwatch.is419(this.delay.getValue().intValue())) {
            CraftingScreenHandler craftingScreenHandler = (CraftingScreenHandler)(minecraftClient.player.currentScreenHandler);
            ItemStack result = recipeEntry834.value().getResult(minecraftClient.world.getRegistryManager());
            if (result.isOf(Items.FIREWORK_ROCKET)) {
                int i = -1;
                int i2 = -1;
                for (int i3 = 10; i3 < 46; i3++) {
                    ItemStack stack = craftingScreenHandler.getSlot(i3).getStack();
                    if (stack.isOf(Items.GUNPOWDER) && i == -1) {
                        i = i3;
                    }
                    if (stack.isOf(Items.PAPER) && i2 == -1) {
                        i2 = i3;
                    }
                }
                if (i2 != -1 && craftingScreenHandler.getSlot(1).getStack().isEmpty()) {
                    this.stopwatch.reset();
                    FireworksHelper.do441(i2, 1);
                    return;
                }
                int[] iArr = {2, 4, 5};
                boolean z = true;
                for (int i4 : iArr) {
                    if (!craftingScreenHandler.getSlot(i4).getStack().isOf(Items.GUNPOWDER)) {
                        z = false;
                    }
                }
                if (i != -1 && !z) {
                    minecraftClient.interactionManager.clickSlot(craftingScreenHandler.syncId, i, 0, SlotActionType.PICKUP, minecraftClient.player);
                    int length = iArr.length;
                    int i5 = 0;
                    while (true) {
                        if (i5 >= length) {
                            break;
                        }
                        int i6 = iArr[i5];
                        if (craftingScreenHandler.getSlot(i6).getStack().isOf(Items.GUNPOWDER)) {
                            i5++;
                        } else if (!craftingScreenHandler.getCursorStack().isEmpty()) {
                            minecraftClient.interactionManager.clickSlot(craftingScreenHandler.syncId, i6, 1, SlotActionType.PICKUP, minecraftClient.player);
                        }
                    }
                    minecraftClient.interactionManager.clickSlot(craftingScreenHandler.syncId, i, 0, SlotActionType.PICKUP, minecraftClient.player);
                    this.stopwatch.reset();
                    return;
                }
                ItemStack stack2 = craftingScreenHandler.getSlot(0).getStack();
                FireworksComponent fireworksComponent = (FireworksComponent) stack2.getOrDefault(DataComponentTypes.FIREWORKS, new FireworksComponent(0, Collections.emptyList()));
                if (!stack2.isOf(Items.FIREWORK_ROCKET) || fireworksComponent.flightDuration() != 3 || !z || !this.stopwatch.is419(Math.max(Helper_7.num, this.delay.getValue().intValue()))) {
                    return;
                }
            } else {
                for (int i7 = 0; i7 < get836(result.getItem()); i7++) {
                    minecraftClient.interactionManager.clickRecipe(craftingScreenHandler.syncId, recipeEntry834, this.craftAll.getValue().booleanValue());
                }
            }
            minecraftClient.interactionManager.clickSlot(craftingScreenHandler.syncId, 0, 1, this.drop.getValue().getSlotActionType2107(), minecraftClient.player);
            if (this.drop.getValue() == AutoCraftMode.Pickup && result.getCount() + craftingScreenHandler.getCursorStack().getCount() > craftingScreenHandler.getCursorStack().getMaxCount()) {
                minecraftClient.interactionManager.clickSlot(craftingScreenHandler.syncId, -999, 0, SlotActionType.PICKUP, minecraftClient.player);
            }
            if (this.fastClose.getValue().booleanValue()) {
                BaritoneHelper_3.tooltipsSearchHelper4_2.do164(() -> {
                    AutoSignSearchHelper4.do2571(new CloseHandledScreenC2SPacket(minecraftClient.player.currentScreenHandler.syncId));
                    minecraftClient.player.currentScreenHandler = minecraftClient.player.playerScreenHandler;
                }, 1);
            }
            this.stopwatch.reset();
        }
    }

    @Listen
    public void onSetScreenHookPre(SetScreenHookPreEvent setScreenHookPreEvent) {
        if ((setScreenHookPreEvent.getScreen247() instanceof CraftingScreen) && this.fastClose.getValue().booleanValue()) {
            setScreenHookPreEvent.getScreen247().init(minecraftClient, 0, 0);
            setScreenHookPreEvent.do1162();
        }
    }

    public RecipeEntry<?> getRecipeEntry834() {
        if (this.items.getValue().isEmpty()) {
            return null;
        }
        for (RecipeEntry<?> recipeEntry2 : minecraftClient.world.getRecipeManager().values()) {
            if (recipeEntry2.value().getType() == RecipeType.CRAFTING) {
                ItemStack result = recipeEntry2.value().getResult(minecraftClient.world.getRegistryManager());
                if (result != null && result.getItem() != null && !(result.getItem() instanceof SmithingTemplateItem)) {
                    List<Ingredient> ingredients = recipeEntry2.value().getIngredients();
                    if (result.isOf(Items.FIREWORK_ROCKET)) {
                        ingredients = list;
                    }
                    if (this.items.getValue().contains(result.getItem()) && is837(ingredients)) {
                        if (is835(result.getItem())) {
                            return recipeEntry2;
                        }
                    }
                }
            }
        }
        return null;
    }

    public boolean is835(Item item) {
        if (!this.limit.getValue().booleanValue() || this.craftAll.getValue().booleanValue()) {
            return true;
        }
        int i = 0;
        int intValue = this.max.getValue().intValue();
        if (this.stackLimit.getValue().booleanValue()) {
            intValue *= item.getMaxCount();
        }
        Iterator it = minecraftClient.player.getInventory().main.iterator();
        while (it.hasNext()) {
            ItemStack itemStack = (ItemStack) it.next();
            if (itemStack.isOf(item)) {
                i += itemStack.getCount();
            }
        }
        return i < intValue;
    }

    public int get836(Item item) {
        if (!this.limit.getValue().booleanValue()) {
            return this.frequency.getValue().intValue();
        }
        int i = 0;
        int intValue = this.max.getValue().intValue();
        if (this.stackLimit.getValue().booleanValue()) {
            intValue *= item.getMaxCount();
        }
        Iterator it = minecraftClient.player.getInventory().main.iterator();
        while (it.hasNext()) {
            ItemStack itemStack = (ItemStack) it.next();
            if (itemStack.isOf(item)) {
                i += itemStack.getCount();
            }
        }
        return MathHelper.clamp(intValue - i, 0, this.frequency.getValue().intValue());
    }

    public boolean is837(Collection<Ingredient> collection) {
        HashMap<Item, Integer> hashMap = new HashMap<>();
        Iterator it = minecraftClient.player.currentScreenHandler.slots.iterator();
        while (it.hasNext()) {
            Slot slot = (Slot) it.next();
            hashMap.compute(slot.getStack().getItem(), (item, num) -> {
                return (item == null || num == null) ? Integer.valueOf(slot.getStack().getCount()) : Integer.valueOf(slot.getStack().getCount() + num.intValue());
            });
        }
        for (Ingredient ingredient : collection) {
            if (!ingredient.isEmpty()) {
                boolean z = false;
                ItemStack[] matchingStacks = ingredient.getMatchingStacks();
                int length = matchingStacks.length;
                int i = 0;
                while (true) {
                    if (i >= length) {
                        break;
                    }
                    ItemStack itemStack = matchingStacks[i];
                    if (this.recipeBlackList.getValue().contains(itemStack.getItem())) {
                        return false;
                    }
                    if (hashMap.containsKey(itemStack.getItem())) {
                        Integer num2 = (Integer) hashMap.get(itemStack.getItem());
                        if (num2.intValue() - itemStack.getCount() >= 0) {
                            hashMap.replace(itemStack.getItem(), Integer.valueOf(num2.intValue() - itemStack.getCount()));
                            z = true;
                            break;
                        }
                    }
                    i++;
                }
                if (!z) {
                    return false;
                }
            }
        }
        return true;
    }

    public HoleSnapData_2<BlockPos, Direction> getHoleSnapData_2838() {
        for (BlockPos blockPos : SearchHelper4_7.getList2429(minecraftClient.player.getEyePos(), this.range.getValue().floatValue(), true)) {
            if (minecraftClient.world.getBlockState(blockPos).isOf(Blocks.CRAFTING_TABLE) && (SearchHelper4_7.is2432(me.mioclient.AutoCraftMode.X8.getList899(blockPos)) || !this.raytrace.getValue().booleanValue())) {
                List<Direction> list3031 = PhaseESPSearchHelper4_2.getList3031(blockPos);
                if (!list3031.isEmpty() || !this.strictDirection.getValue().booleanValue()) {
                    return new HoleSnapData_2<>(blockPos, list3031.isEmpty() ? Direction.UP : list3031.get(0));
                }
            }
        }
        return null;
    }
}
