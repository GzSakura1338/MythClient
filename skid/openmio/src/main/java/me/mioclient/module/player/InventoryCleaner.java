package me.mioclient.module.player;

import java.util.Iterator;
import java.util.Set;
import me.mioclient.Helper_7;
import me.mioclient.PhaseESPHelper;
import me.mioclient.ScaffoldMode_2;
import me.mioclient.TooltipsShulkerBoxScreen;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.ingame.GenericContainerScreen;
import net.minecraft.client.gui.screen.ingame.InventoryScreen;
import net.minecraft.client.gui.screen.ingame.ShulkerBoxScreen;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.PlayerScreenHandler;
import net.minecraft.screen.slot.Slot;
import net.minecraft.screen.slot.SlotActionType;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/InventoryCleaner.class */
public class InventoryCleaner extends Module {
    public Setting<Set<Item>> whitelist;
    public Setting<ScaffoldMode_2> selection;
    public Setting<Integer> delay;
    public Setting<Integer> frequency;
    public Setting<Boolean> onlyUI;
    public Setting<Boolean> ignoreHotbar;
    public final Stopwatch stopwatch;

    public InventoryCleaner() {
        super("InventoryCleaner", "Gets rid of the selected items in your inventory.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
    }

    @Listen(get219= Helper_7.num2)
    public void do27(TickEvent tickEvent) {
        int i = 0;
        if (this.stopwatch.is419(this.delay.getValue().intValue())) {
            Screen screen = minecraftClient.currentScreen;
            boolean z = (screen instanceof InventoryScreen) || (screen instanceof GenericContainerScreen) || ((screen instanceof ShulkerBoxScreen) && !(screen instanceof TooltipsShulkerBoxScreen));
            if (!this.onlyUI.getValue().booleanValue() || z) {
                int size = minecraftClient.player.currentScreenHandler.slots.size();
                if (minecraftClient.player.currentScreenHandler instanceof PlayerScreenHandler) {
                    size--;
                }
                Iterator it = minecraftClient.player.currentScreenHandler.slots.iterator();
                while (it.hasNext()) {
                    Slot slot = (Slot) it.next();
                    ItemStack stack = slot.getStack();
                    if (!stack.isEmpty()) {
                        if (this.selection.getValue().is1392(stack.getItem(), this.whitelist) && (slot.id < size - 9 || !this.ignoreHotbar.getValue().booleanValue())) {
                            minecraftClient.interactionManager.clickSlot(minecraftClient.player.currentScreenHandler.syncId, slot.id, 1, SlotActionType.THROW, minecraftClient.player);
                            this.stopwatch.reset();
                            i++;
                            if (i >= this.frequency.getValue().intValue()) {
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
}
