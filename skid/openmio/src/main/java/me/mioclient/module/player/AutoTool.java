package me.mioclient.module.player;

import me.mioclient.BreakingProgressHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.module.Module;
import net.minecraft.block.BlockState;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoTool.class */
public class AutoTool extends Module {
    public Setting<Boolean> swapBack;
    public int num;
    public boolean flag;

    public AutoTool() {
        super("AutoTool", "Equips the best tool to use on a block for you.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (((BreakingProgressHelper)(Object) minecraftClient.interactionManager).getCurrentBreakingBlock() == null || !minecraftClient.options.attackKey.isPressed()) {
            if (this.swapBack.getValue().booleanValue()) {
                if (!this.flag) {
                    this.num = minecraftClient.player.getInventory().selectedSlot;
                    return;
                }
                minecraftClient.player.getInventory().selectedSlot = this.num;
                this.flag = false;
                return;
            }
            return;
        }
        this.flag = true;
        int i = minecraftClient.player.getInventory().selectedSlot;
        BlockState blockState = minecraftClient.world.getBlockState(((BreakingProgressHelper)(Object) minecraftClient.interactionManager).getCurrentBreakingBlock());
        double miningSpeedMultiplier = minecraftClient.player.getInventory().getStack(i).getMiningSpeedMultiplier(blockState);
        for (int i2 = 0; i2 < 9; i2++) {
            double miningSpeedMultiplier2 = minecraftClient.player.getInventory().getStack(i2).getMiningSpeedMultiplier(blockState);
            if (miningSpeedMultiplier2 > miningSpeedMultiplier) {
                miningSpeedMultiplier = miningSpeedMultiplier2;
                i = i2;
            }
        }
        FireworksHelper.do456(i);
    }
}
