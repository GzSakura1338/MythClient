package me.mioclient.module.player;

import me.mioclient.EnumSettingHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.block.BedBlock;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.item.EnderPearlItem;
import net.minecraft.item.ItemStack;
import net.minecraft.item.ProjectileItem;
import net.minecraft.item.RangedWeaponItem;
import net.minecraft.util.Hand;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/NoInteract.class */
public class NoInteract extends Module {
    public Setting<Mode> mode;
    public Setting<Boolean> onlyFood;
    public boolean flag;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/player/NoInteract$Mode.class */
    public enum Mode implements EnumSettingHelper {
        CANCEL("Cancel"),
        SHIFT("Shift");

        public final String name;

        Mode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public NoInteract() {
        super("NoInteract", "Prevents you from interacting with tile entities.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    public static boolean is1840(ItemStack itemStack) {
        return itemStack.contains(DataComponentTypes.FOOD) || (itemStack.getItem() instanceof EnderPearlItem) || (itemStack.getItem() instanceof ProjectileItem) || (itemStack.getItem() instanceof RangedWeaponItem);
    }

    public static Hand getHand1841() {
        return (is1840(minecraftClient.player.getMainHandStack()) || !is1840(minecraftClient.player.getOffHandStack())) ? Hand.MAIN_HAND : Hand.OFF_HAND;
    }

    public boolean is1842(ItemStack itemStack, BlockPos blockPos) {
        if (!isToggled()) {
            return false;
        }
        if (this.onlyFood.getValue().booleanValue() && !itemStack.contains(DataComponentTypes.FOOD)) {
            return false;
        }
        BlockState blockState = minecraftClient.world.getBlockState(blockPos);
        if (blockState.isOf(Blocks.ANVIL)) {
            return true;
        }
        return blockState.hasBlockEntity() && !(blockState.getBlock() instanceof BedBlock);
    }
}
