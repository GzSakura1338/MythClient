package me.mioclient.module.player;

import me.mioclient.MatrixStackEvent_2;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import net.minecraft.client.gui.screen.ingame.GenericContainerScreen;
import net.minecraft.client.gui.screen.ingame.HandledScreen;
import net.minecraft.client.gui.screen.ingame.ShulkerBoxScreen;
import net.minecraft.screen.slot.Slot;
import net.minecraft.screen.slot.SlotActionType;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/InventoryTweaks.class */
public class InventoryTweaks extends Module {
    public Setting<Boolean> fastMove;
    public Setting<Boolean> onlyContainer;
    public Setting<Boolean> fastArmor;
    public Setting<Boolean> xCarryTweaks;
    public int num;

    public InventoryTweaks() {
        super("InventoryTweaks", "Modifies your inventory actions.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void onEvent(MatrixStackEvent_2 matrixStackEvent_2) {
        Slot mio$getFocusedSlot;
        if (this.fastMove.getValue().booleanValue() && (minecraftClient.currentScreen instanceof HandledScreen)) {
            if ((this.onlyContainer.getValue().booleanValue() && !(minecraftClient.currentScreen instanceof GenericContainerScreen) && !(minecraftClient.currentScreen instanceof ShulkerBoxScreen)) || !minecraftClient.player.currentScreenHandler.getCursorStack().isEmpty() || (mio$getFocusedSlot = ((me.mioclient.mixin.ducks.DuckHandledScreen)(Object) minecraftClient.currentScreen).mio$getFocusedSlot()) == null || mio$getFocusedSlot.id == this.num || mio$getFocusedSlot.getStack().isEmpty()) {
                return;
            }
            int i = -1;
            if (GLFW.glfwGetMouseButton(minecraftClient.getWindow().getHandle(), 0) == 1) {
                i = 0;
            }
            if (GLFW.glfwGetKey(minecraftClient.getWindow().getHandle(), 340) != 1 || i == -1) {
                return;
            }
            minecraftClient.interactionManager.clickSlot(minecraftClient.player.currentScreenHandler.syncId, mio$getFocusedSlot.id, i, SlotActionType.QUICK_MOVE, minecraftClient.player);
            this.num = mio$getFocusedSlot.id;
        }
    }
}
