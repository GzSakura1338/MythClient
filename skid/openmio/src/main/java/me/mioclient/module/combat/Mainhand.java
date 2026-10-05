package me.mioclient.module.combat;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.EntityControlSearchHelper4;
import me.mioclient.FireworksHelper;
import me.mioclient.Helper_7;
import me.mioclient.MainhandHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper4_7;
import me.mioclient.SearchHelper_3;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.InteractBlockEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.RenderEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import me.mioclient.module.player.NoInteract;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.Hand;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/Mainhand.class */
public class Mainhand extends Module {
    public static final NoInteract noInteract = (NoInteract) BaritoneHelper_3.baritoneHelper_4.getModule117(NoInteract.class);
    public Setting<Integer> slot;
    public Setting<Integer> delay;
    public Setting<Boolean> autoSelect;
    public Setting<Integer> delay2;
    public Setting<Boolean> lethal;
    public Setting<Float> health;
    public Setting<Boolean> gappleBind;
    public Setting<Float> safe;
    public final Stopwatch stopwatch;
    public final Stopwatch stopwatch2;
    public final Stopwatch stopwatch3;
    public boolean flag;
    public int num;
    public final MainhandHelper mainhandHelper;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/combat/Mainhand$Inner.class */
    class Inner extends MainhandHelper {
        public Inner() {
        }

        @Override // me.mioclient.MainhandHelper
        public boolean is465() {
            return Mainhand.this.lethal.getValue().booleanValue() && Mainhand.this.autoSelect.getValue().booleanValue();
        }
    }

    public Mainhand() {
        super("Mainhand", "Auto totem for main hand.", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
        this.stopwatch2 = new Stopwatch();
        this.stopwatch3 = new Stopwatch();
        this.mainhandHelper = new Inner();
        this.delay2.do2329("SwapDelay");
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        this.num = -1;
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        return String.valueOf(minecraftClient.player.getInventory().main.stream().filter(itemStack -> {
            return itemStack.getItem() == getItem581();
        }).mapToInt((v0) -> {
            return v0.getCount();
        }).sum());
    }

    @Listen
    public void onInteractBlock(InteractBlockEvent interactBlockEvent) {
        if (!((!SearchHelper4_7.is2450(interactBlockEvent.getBlockHitResult2585()) || minecraftClient.player.isSneaking() || noInteract.isToggled()) ? false : true) || interactBlockEvent.is2403()) {
            return;
        }
        this.stopwatch3.reset();
    }

    @Listen(get219= Helper_7.num2)
    public void onRender(RenderEvent renderEvent) {
        if (is1469()) {
            return;
        }
        this.flag = renderEvent.is168();
        this.mainhandHelper.do466();
        if (minecraftClient.player.currentScreenHandler != minecraftClient.player.playerScreenHandler) {
            return;
        }
        do2116();
        Item item581 = getItem581();
        if (item581 != minecraftClient.player.getInventory().getStack(get2118()).getItem() && minecraftClient.player.currentScreenHandler.getCursorStack().isEmpty()) {
            do2115(item581);
        }
    }

    public void do2115(Item item) {
        int i = FireworksHelper.get445(itemStack -> {
            return itemStack.isOf(item);
        }, true);
        if (this.num != -1) {
            if (minecraftClient.player.getInventory().getStack(this.num).isOf(item) && i != -1) {
                i = this.num;
            }
        }
        if (i != -1 && i < minecraftClient.player.currentScreenHandler.slots.size()) {
            ItemStack stack = minecraftClient.player.currentScreenHandler.getSlot(i).getStack();
            if (!stack.contains(DataComponentTypes.FOOD) || this.flag) {
                if (this.stopwatch.is420(this.delay.getValue().intValue()) && this.stopwatch3.is419(200L)) {
                    FireworksHelper.do440(i, get2118());
                    this.num = i;
                }
                if (EntityControlSearchHelper4.is1538(minecraftClient.options.useKey) && stack.contains(DataComponentTypes.FOOD)) {
                    minecraftClient.interactionManager.interactItem(minecraftClient.player, Hand.OFF_HAND);
                }
            }
        }
    }

    public void do2116() {
        if (this.autoSelect.getValue().booleanValue()) {
            if (minecraftClient.player.getInventory().getStack(get2118()).getItem() == Items.TOTEM_OF_UNDYING) {
                if (minecraftClient.player.isUsingItem() && minecraftClient.player.getActiveHand() == Hand.MAIN_HAND) {
                    return;
                }
                if (this.mainhandHelper.is467() || SearchHelper_3.get643() <= this.health.getValue().floatValue()) {
                    if (this.stopwatch2.is420(this.delay2.getValue().intValue())) {
                        FireworksHelper.do456(get2118());
                    }
                }
            }
        }
    }

    /* JADX WARN: Code restructure failed: missing block: B:12:0x0075, code lost:
    
        if (me.mioclient.module.combat.Mainhand.minecraftClient.player.getMainHandStack().contains(net.minecraft.component.DataComponentTypes.FOOD) == false) goto L14;
     */
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    public Item getItem581() {
        boolean z = SearchHelper4_7.is2450(minecraftClient.crosshairTarget) && !minecraftClient.player.isSneaking() && !noInteract.isToggled()
                && minecraftClient.player.isUsingItem() && minecraftClient.player.getActiveHand() == Hand.MAIN_HAND;
        return (minecraftClient.player.getInventory().selectedSlot == get2118() || !this.gappleBind.getValue().booleanValue() || !minecraftClient.options.useKey.isPressed() || !is2117() || z || SearchHelper_3.get643() < this.safe.getValue().floatValue()) ? Items.TOTEM_OF_UNDYING : Items.ENCHANTED_GOLDEN_APPLE;
    }

    public boolean is2117() {
        ItemStack mainHandStack = minecraftClient.player.getMainHandStack();
        if (minecraftClient.player.getOffHandStack().contains(DataComponentTypes.FOOD)) {
            return false;
        }
        return mainHandStack.isOf(Items.TOTEM_OF_UNDYING) || mainHandStack.isOf(Items.ENCHANTED_GOLDEN_APPLE);
    }

    public int get2118() {
        return this.slot.getValue().intValue() - 1;
    }
}
