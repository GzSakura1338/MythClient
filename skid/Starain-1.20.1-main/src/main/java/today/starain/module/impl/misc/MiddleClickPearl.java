package today.starain.module.impl.misc;

import net.minecraft.world.InteractionHand;
import net.minecraft.world.inventory.ClickType;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import org.lwjgl.glfw.GLFW;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.event.impl.MouseButtonEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.value.impl.BooleanValue;

public class MiddleClickPearl extends ClientModule {

    private final BooleanValue wholeInventory = new BooleanValue("Whole Inventory", true, this);
    private final BooleanValue swingHand = new BooleanValue("Swing", true, this);

    private boolean throwPending;
    private int restoreSlot = -1;
    private int swapBackSlot = -1;

    public MiddleClickPearl() {
        setName("MiddleClickPearl");
        setCategory(Category.MISC);
    }

    @Override
    public void onDisable() {
        throwPending = false;
        restoreSlot = -1;
        swapBackSlot = -1;
    }

    @EventTarget
    public void onMouse(MouseButtonEvent event) {
        if (event.button() == GLFW.GLFW_MOUSE_BUTTON_MIDDLE
                && event.action() == GLFW.GLFW_PRESS
                && event.screen() == null) {
            throwPending = true;
        }
    }

    @EventTarget
    public void onTick(GameTickEvent event) {
        if (event.getType() != EventType.PRE || mc.player == null || mc.gameMode == null) {
            return;
        }

        if (swapBackSlot != -1) {
            mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId,
                    swapBackSlot, mc.player.getInventory().selected, ClickType.SWAP, mc.player);
            swapBackSlot = -1;
        }
        if (restoreSlot != -1) {
            mc.player.getInventory().selected = restoreSlot;
            restoreSlot = -1;
        }

        if (!throwPending) {
            return;
        }
        throwPending = false;

        if (mc.player.getOffhandItem().getItem() == Items.ENDER_PEARL) {
            use(InteractionHand.OFF_HAND);
            return;
        }

        int hotbar = findHotbarPearl();
        if (hotbar != -1) {
            restoreSlot = mc.player.getInventory().selected;
            mc.player.getInventory().selected = hotbar;
            use(InteractionHand.MAIN_HAND);
            return;
        }

        if (wholeInventory.get()) {
            int invSlot = findInventoryPearl();
            if (invSlot != -1) {
                int selected = mc.player.getInventory().selected;
                mc.gameMode.handleInventoryMouseClick(mc.player.inventoryMenu.containerId,
                        invSlot, selected, ClickType.SWAP, mc.player);
                use(InteractionHand.MAIN_HAND);
                swapBackSlot = invSlot;
            }
        }
    }

    private void use(InteractionHand hand) {
        mc.gameMode.useItem(mc.player, hand);
        if (swingHand.get()) {
            mc.player.swing(hand);
        }
    }

    private int findHotbarPearl() {
        for (int i = 0; i < 9; i++) {
            if (mc.player.getInventory().getItem(i).getItem() == Items.ENDER_PEARL) {
                return i;
            }
        }
        return -1;
    }

    private int findInventoryPearl() {
        for (int i = 9; i < 36; i++) {
            ItemStack stack = mc.player.getInventory().getItem(i);
            if (stack.getItem() == Items.ENDER_PEARL) {
                return i;
            }
        }
        return -1;
    }
}
