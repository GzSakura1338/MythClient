package me.mioclient.module.player;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import me.mioclient.AutoCrystalHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ChestStealerData;
import me.mioclient.ChestStealerEnumSettingHelper;
import me.mioclient.ChestStealerHelper;
import me.mioclient.ChestStealerMode;
import me.mioclient.ChestStealerMode_2;
import me.mioclient.ChestStealerSearchHelper4;
import me.mioclient.ChestStealerSearchHelper4_2;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PositionData;
import me.mioclient.ScaffoldMode_2;
import me.mioclient.TooltipsShulkerBoxScreen;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.InsertItemEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.SetScreenHookPostEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Event_2;
import me.mioclient.feature.Stopwatch;
import me.mioclient.mixin.ducks.DuckHandledScreen;
import me.mioclient.module.Module;
import me.mioclient.module.misc.ChestSearchBar;
import net.fabricmc.fabric.api.client.screen.v1.ScreenEvents;
import net.minecraft.client.gui.screen.ingame.GenericContainerScreen;
import net.minecraft.client.gui.screen.ingame.ShulkerBoxScreen;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.ShulkerBoxScreenHandler;
import net.minecraft.screen.slot.Slot;
import net.minecraft.screen.slot.SlotActionType;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/ChestStealer.class */
public class ChestStealer extends Module {
    public static ChestSearchBar chestSearchBar;
    public Setting<Set<Item>> whitelist;
    public Setting<Set<String>> names;
    public Setting<ChestStealerMode> mode;
    public Setting<ScaffoldMode_2> select;
    public Setting<ChestStealerMode_2> lookFor;
    public Setting<Boolean> searchBarSync;
    public Setting<Boolean> onlyFull;
    public Setting<Boolean> onlyButtons;
    public Setting<Boolean> animate;
    public Setting<Integer> delay;
    public Setting<Integer> timeout;
    public Setting<Integer> frequency;
    public final List<ChestStealerSearchHelper4_2> list;
    public final Stopwatch stopwatch;
    public final Stopwatch stopwatch2;
    public ChestStealerMode chestStealerMode;
    public boolean flag;
    public Slot slot;
    public static final /* synthetic */ boolean flag2;

    public ChestStealer() {
        super("ChestStealer", "Steals items from the storages you open.", Category.PLAYER, "stealer");
        PhaseESPHelper.do1351(this);
        this.list = Collections.synchronizedList(new ArrayList());
        this.stopwatch = new Stopwatch();
        this.stopwatch2 = new Stopwatch();
        Setting<Boolean> setting = this.animate;
        List<ChestStealerSearchHelper4_2> list = this.list;
        Objects.requireNonNull(list);
        setting.do2339(list::clear);
        this.mode.do2343(chestStealerMode -> {
            return !this.onlyButtons.getValue().booleanValue();
        });
        ScreenEvents.AFTER_INIT.register(new ChestStealerSearchHelper4(this));
    }

    /* JADX WARN: Can't fix incorrect switch cases order, some code will duplicate */
    /* JADX WARN: Failed to find 'out' block for switch in B:45:0x0301. Please report as an issue. */
    /* JADX WARN: Removed duplicated region for block: B:101:0x0738 A[PHI: r16
      0x0738: PHI (r16v3 int) = (r16v1 int), (r16v5 int), (r16v7 int), (r16v8 int), (r16v1 int), (r16v1 int), (r16v1 int) binds: [B:45:0x0301, B:89:0x0732, B:61:0x0443, B:57:0x041c, B:53:0x03cb, B:54:0x03ce, B:46:0x0324] A[DONT_GENERATE, DONT_INLINE]] */
    /* JADX WARN: Removed duplicated region for block: B:103:0x0772 A[SYNTHETIC] */
    /* JADX WARN: Removed duplicated region for block: B:105:0x076c A[SYNTHETIC] */
    /* JADX WARN: Removed duplicated region for block: B:46:0x0324  */
    /* JADX WARN: Removed duplicated region for block: B:47:0x035f  */
    /* JADX WARN: Removed duplicated region for block: B:55:0x0414  */
    /* JADX WARN: Removed duplicated region for block: B:59:0x043e  */
    /* JADX WARN: Removed duplicated region for block: B:90:0x076c A[SYNTHETIC] */
    @Listen
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    public void do27(TickEvent tickEvent) {
        if ((minecraftClient.currentScreen instanceof GenericContainerScreen) || (minecraftClient.currentScreen instanceof ShulkerBoxScreen)) {
            if (this.stopwatch.is419(this.delay.getValue().intValue())) {
                if (this.onlyButtons.getValue().booleanValue() && this.chestStealerMode == null) {
                    return;
                }
                if (!this.stopwatch2.is419(this.timeout.getValue().intValue()) || (minecraftClient.currentScreen instanceof TooltipsShulkerBoxScreen)) {
                    return;
                }
                ChestStealerMode value = this.onlyButtons.getValue().booleanValue() ? this.chestStealerMode : this.mode.getValue();
                ScreenHandler screenHandler = minecraftClient.player.currentScreenHandler;
                int i = 0;
                int i2 = screenHandler.syncId;
                GenericContainerScreen genericContainerScreen = (minecraftClient.currentScreen) instanceof GenericContainerScreen ? (GenericContainerScreen) (minecraftClient.currentScreen) : null;
                int rows = genericContainerScreen instanceof GenericContainerScreen ? 9 * genericContainerScreen.getScreenHandler().getRows() : 27;
                DuckHandledScreen duckHandledScreen = (DuckHandledScreen)(minecraftClient.currentScreen);
                Map<ChestStealerData, List<ChestStealerHelper>> map2360 = getMap2360(rows);
                if (value == ChestStealerMode.FILL) {
                    i = rows;
                    rows = 36;
                }
                int i3 = 0;
                for (int i4 = i; i4 < i + rows; i4++) {
                    ItemStack stack = screenHandler.getSlot(i4).getStack();
                    if (!stack.isEmpty()) {
                        if (this.searchBarSync.getValue().booleanValue() && chestSearchBar.isToggled()) {
                            if (!chestSearchBar.match(stack, this.onlyFull.getValue().booleanValue())) {
                                continue;
                            }
                        }
                        boolean z = minecraftClient.currentScreen instanceof ShulkerBoxScreen;
                        if (this.lookFor.getValue() == ChestStealerMode_2.ITEM) {
                            if (!this.select.getValue().is1392(stack.getItem(), this.whitelist)) {
                                continue;
                            }
                            List<ChestStealerHelper> list = map2360.get(new ChestStealerData(stack.getItem(), stack.getComponents()));
                            switch (value) {
                                case DROP:
                                    minecraftClient.interactionManager.clickSlot(i2, i4, 1, SlotActionType.THROW, minecraftClient.player);
                                    this.stopwatch.reset();
                                    i3++;
                                    if (i3 < this.frequency.getValue().intValue()) {
                                        break;
                                    } else {
                                        return;
                                    }
                                case FILL:
                                case STEAL:
                                    if (z && AutoCrystalHelper.is135(stack.getItem())) {
                                        break;
                                    } else {
                                        this.slot = null;
                                        this.flag = true;
                                        minecraftClient.interactionManager.clickSlot(i2, i4, 0, SlotActionType.QUICK_MOVE, minecraftClient.player);
                                        this.flag = false;
                                        if (this.slot != null) {
                                            do2359(screenHandler.getSlot(i4), this.slot, this.slot.getStack());
                                        }
                                        this.stopwatch.reset();
                                        i3++;
                                        if (i3 < this.frequency.getValue().intValue()) {
                                        }
                                    }
                                    break;
                                case REFILL:
                                    if (list == null) {
                                        continue;
                                    } else {
                                        do2357(i4, list);
                                        i3 = 999;
                                        this.stopwatch.reset();
                                        i3++;
                                        if (i3 < this.frequency.getValue().intValue()) {
                                        }
                                    }
                                    break;
                                case REKIT:
                                    if (list != null) {
                                        do2357(i4, list);
                                        i3 = 999;
                                    } else {
                                        ChestStealerEnumSettingHelper chestStealerEnumSettingHelper257 = BaritoneHelper_3.chestStealerSearchHelper4_3.getChestStealerEnumSettingHelper257();
                                        if (chestStealerEnumSettingHelper257 == null) {
                                            continue;
                                        } else {
                                            Slot slot = screenHandler.getSlot(i4);
                                            boolean z2 = false;
                                            for (Map.Entry<Integer, ChestStealerEnumSettingHelper.Record> entry : chestStealerEnumSettingHelper257.getMap2747().entrySet()) {
                                                if (entry.getValue().is774(stack)) {
                                                    boolean z3 = entry.getKey().intValue() < 9;
                                                    int intValue = rows + (z3 ? entry.getKey().intValue() + 27 : entry.getKey().intValue() - 9);
                                                    ItemStack stack2 = screenHandler.getSlot(intValue).getStack();
                                                    if (!entry.getValue().is774(stack2) && (!AutoCrystalHelper.is135(stack2.getItem()) || !(screenHandler instanceof ShulkerBoxScreenHandler))) {
                                                        do2359(slot, screenHandler.getSlot(intValue), slot.getStack());
                                                        if (z3) {
                                                            minecraftClient.interactionManager.clickSlot(i2, i4, entry.getKey().intValue(), SlotActionType.SWAP, minecraftClient.player);
                                                        } else {
                                                            minecraftClient.interactionManager.clickSlot(i2, i4, 0, SlotActionType.PICKUP, minecraftClient.player);
                                                            minecraftClient.interactionManager.clickSlot(i2, intValue, 0, SlotActionType.PICKUP, minecraftClient.player);
                                                            minecraftClient.interactionManager.clickSlot(i2, i4, 0, SlotActionType.PICKUP, minecraftClient.player);
                                                        }
                                                        i3 = 999;
                                                        z2 = true;
                                                        if (!z2) {
                                                            continue;
                                                        }
                                                    }
                                                }
                                            }
                                            if (!z2) {
                                            }
                                        }
                                    }
                                    this.stopwatch.reset();
                                    i3++;
                                    if (i3 < this.frequency.getValue().intValue()) {
                                    }
                                    break;
                                default:
                                    this.stopwatch.reset();
                                    i3++;
                                    if (i3 < this.frequency.getValue().intValue()) {
                                    }
                                    break;
                            }
                        } else {
                            if (!this.select.getValue().is1392(stack.getName().getString().toLowerCase(Locale.ROOT), this.names)) {
                                continue;
                            }
                            List<ChestStealerHelper> list2 = map2360.get(new ChestStealerData(stack.getItem(), stack.getComponents()));
                            switch (value) {
                            }
                        }
                    }
                }
            }
        }
    }

    @Listen
    public void onSetScreenHookPost(SetScreenHookPostEvent setScreenHookPostEvent) {
        this.stopwatch2.reset();
        this.list.clear();
    }

    @Listen
    public void onRender(Event_2 event_2) {
        synchronized (this.list) {
            this.list.removeIf((v0) -> {
                return v0.is2551();
            });
            Iterator<ChestStealerSearchHelper4_2> it = this.list.iterator();
            while (it.hasNext()) {
                it.next().do2548(event_2.getDrawContext474());
            }
        }
    }

    @Listen
    public void onInsertItem(InsertItemEvent insertItemEvent) {
        if (!this.flag || insertItemEvent.getSlot2783().getStack().isEmpty()) {
            return;
        }
        this.slot = insertItemEvent.getSlot2783();
    }

    public void do2357(int i, List<ChestStealerHelper> list) {
        Slot slot = minecraftClient.player.currentScreenHandler.getSlot(i);
        if (slot.getStack().isStackable()) {
            int i2 = minecraftClient.player.currentScreenHandler.syncId;
            int count = slot.getStack().getCount();
            int i3 = 0;
            minecraftClient.interactionManager.clickSlot(i2, i, 0, SlotActionType.PICKUP, minecraftClient.player);
            for (ChestStealerHelper chestStealerHelper : list) {
                Slot slot2 = minecraftClient.player.currentScreenHandler.getSlot(chestStealerHelper.get499());
                minecraftClient.interactionManager.clickSlot(i2, chestStealerHelper.get499(), 0, SlotActionType.PICKUP, minecraftClient.player);
                int i4 = chestStealerHelper.get3063() - chestStealerHelper.get3064();
                chestStealerHelper.do3065(chestStealerHelper.get3064() + count);
                count -= i4;
                do2359(minecraftClient.player.currentScreenHandler.getSlot(i), slot2, slot2.getStack().copyWithCount(i4));
                if (minecraftClient.player.currentScreenHandler.getCursorStack().isEmpty()) {
                    break;
                }
                i3++;
                if (i3 >= this.frequency.getValue().intValue()) {
                    break;
                }
            }
            if (minecraftClient.player.currentScreenHandler.getCursorStack().isEmpty()) {
                return;
            }
            minecraftClient.interactionManager.clickSlot(i2, i, 0, SlotActionType.PICKUP, minecraftClient.player);
        }
    }

    public boolean is2358(ItemStack itemStack) {
        return ((minecraftClient.player.currentScreenHandler instanceof ShulkerBoxScreenHandler) && AutoCrystalHelper.is135(itemStack.getItem())) ? false : true;
    }

    public void do2359(Slot slot, Slot slot2, ItemStack itemStack) {
        DuckHandledScreen duckHandledScreen = (DuckHandledScreen)(minecraftClient.currentScreen);
        if (!flag2 && duckHandledScreen == null) {
            throw new AssertionError();
        }
        if (this.animate.getValue().booleanValue()) {
            this.list.add(new ChestStealerSearchHelper4_2(itemStack, new PositionData(duckHandledScreen.getX() + slot.x, duckHandledScreen.getY() + slot.y), new PositionData(duckHandledScreen.getX() + slot2.x, duckHandledScreen.getY() + slot2.y)));
        }
    }

    public Map<ChestStealerData, List<ChestStealerHelper>> getMap2360(int i) {
        HashMap hashMap = new HashMap();
        for (int i2 = 0; i2 < 36; i2++) {
            if (i2 + i < minecraftClient.player.currentScreenHandler.slots.size()) {
                ItemStack stack = minecraftClient.player.currentScreenHandler.getSlot(i + i2).getStack();
                if (stack.getCount() < stack.getMaxCount() && !stack.isEmpty() && stack.isStackable()) {
                    ChestStealerData chestStealerData = new ChestStealerData(stack.getItem(), stack.getComponents());
                    hashMap.putIfAbsent(chestStealerData, new ArrayList());
                    ((List) hashMap.get(chestStealerData)).add(new ChestStealerHelper(i + i2, stack.getMaxCount(), stack.getCount()));
                }
            }
        }
        return hashMap;
    }

    public void do2361(ChestStealerMode chestStealerMode) {
        this.chestStealerMode = chestStealerMode;
    }

    public ChestStealerMode getChestStealerMode2362() {
        return this.chestStealerMode;
    }

    static {
        flag2 = !ChestStealer.class.desiredAssertionStatus();
        chestSearchBar = (ChestSearchBar) BaritoneHelper_3.baritoneHelper_4.getModule117(ChestSearchBar.class);
    }
}
