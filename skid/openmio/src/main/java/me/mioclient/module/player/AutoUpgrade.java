package me.mioclient.module.player;

import java.util.Iterator;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickPostEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.client.gui.screen.ingame.SmithingScreen;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.ArmorMaterial;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.item.ToolItem;
import net.minecraft.recipe.Ingredient;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.slot.Slot;
import net.minecraft.screen.slot.SlotActionType;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoUpgrade.class */
public class AutoUpgrade extends Module {
    public Setting<AutoUpgradeMode> items;
    public Setting<Integer> delay;
    public Setting<Boolean> safe;
    public final Stopwatch stopwatch;
    public static final int num4 = 3;
    public static final int num3 = 2;
    public static final int num2 = 1;
    public static final int num = 0;
    public static final Item item = Items.NETHERITE_UPGRADE_SMITHING_TEMPLATE;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoUpgrade$AutoUpgradeMode.class */
    public enum AutoUpgradeMode implements EnumSettingHelper {
        ANY("Any"),
        TOOLS("Tools"),
        ARMOR("Armor");

        public final String name;

        AutoUpgradeMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public AutoUpgrade() {
        super("AutoUpgrade", "Automatically upgrades diamond items to netherite.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
    }

    @Listen
    public void onTickPost(TickPostEvent tickPostEvent) {
        ScreenHandler screenHandler = minecraftClient.player.currentScreenHandler;
        if (minecraftClient.currentScreen instanceof SmithingScreen) {
            if (this.stopwatch.is419(this.delay.getValue().intValue())) {
                if (get731(screenHandler, item) > 1 || !this.safe.getValue().booleanValue()) {
                    if (!screenHandler.getSlot(3).getStack().isEmpty()) {
                        minecraftClient.interactionManager.clickSlot(screenHandler.syncId, 3, 0, SlotActionType.QUICK_MOVE, minecraftClient.player);
                        this.stopwatch.reset();
                        return;
                    }
                    ItemStack stack = screenHandler.getSlot(0).getStack();
                    if (stack.isEmpty() || (stack.isOf(item) && stack.getCount() == 1)) {
                        for (int i = 4; i < 40; i++) {
                            if (screenHandler.getSlot(i).getStack().isOf(item)) {
                                FireworksHelper.do441(i, 0);
                                this.stopwatch.reset();
                                return;
                            }
                        }
                        return;
                    }
                    if (screenHandler.getSlot(1).getStack().isEmpty()) {
                        int i2 = get732(screenHandler);
                        if (i2 == -1) {
                            return;
                        }
                        FireworksHelper.do441(i2, 1);
                        this.stopwatch.reset();
                        return;
                    }
                    if (screenHandler.getSlot(2).getStack().isEmpty()) {
                        for (int i3 = 4; i3 < 40; i3++) {
                            if (screenHandler.getSlot(i3).getStack().isOf(Items.NETHERITE_INGOT)) {
                                FireworksHelper.do441(i3, 2);
                                this.stopwatch.reset();
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    public int get731(ScreenHandler screenHandler, Item item2) {
        int i = 0;
        Iterator it = screenHandler.slots.iterator();
        while (it.hasNext()) {
            Slot slot = (Slot) it.next();
            if (slot.getStack().isOf(item2)) {
                i += slot.getStack().getCount();
            }
        }
        return i;
    }

    public int get732(ScreenHandler screenHandler) {
        boolean z = this.items.getValue() == AutoUpgradeMode.TOOLS;
        boolean z2 = this.items.getValue() == AutoUpgradeMode.ARMOR;
        for (int i = 4; i < 40; i++) {
            ItemStack stack = screenHandler.getSlot(i).getStack();
            if (!z) {
                ArmorItem item2 = (stack.getItem()) instanceof ArmorItem ? (ArmorItem) (stack.getItem()) : null;
                if (item2 instanceof ArmorItem) {
                    if (is733((Ingredient) ((ArmorMaterial) item2.getMaterial().value()).repairIngredient().get())) {
                        return i;
                    }
                }
            }
            if (!z2) {
                ToolItem item3 = (stack.getItem()) instanceof ToolItem ? (ToolItem) (stack.getItem()) : null;
                if (item3 instanceof ToolItem) {
                    if (is733(item3.getMaterial().getRepairIngredient())) {
                        return i;
                    }
                } else {
                    continue;
                }
            }
        }
        return -1;
    }

    public boolean is733(Ingredient ingredient) {
        for (ItemStack itemStack : ingredient.getMatchingStacks()) {
            if (itemStack.isOf(Items.DIAMOND)) {
                return true;
            }
        }
        return false;
    }
}
