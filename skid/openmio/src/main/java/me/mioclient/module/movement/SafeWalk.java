package me.mioclient.module.movement;

import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ClipAtLedgeEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/SafeWalk.class */
public class SafeWalk extends Module {
    public Setting<Boolean> sneak;
    public Setting<Integer> delay;
    public final Stopwatch stopwatch;

    public SafeWalk() {
        super("SafeWalk", "Helps not to fall from blocks if you're bad.", Category.MOVEMENT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        if (this.sneak.getValue().booleanValue()) {
            minecraftClient.options.sneakKey.setPressed(false);
        }
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (this.sneak.getValue().booleanValue()) {
            BlockPos down = BlockPos.ofFloored(minecraftClient.player.getPos()).down();
            BlockState blockState = minecraftClient.world.getBlockState(down);
            if (blockState.isOf(Blocks.AIR) || blockState.getCollisionShape(minecraftClient.world, down).isEmpty()) {
                this.stopwatch.reset();
                minecraftClient.options.sneakKey.setPressed(true);
            } else {
                minecraftClient.options.sneakKey.setPressed(!this.stopwatch.is419((long) this.delay.getValue().intValue()));
            }
        }
    }

    @Listen
    public void onClipAtLedge(ClipAtLedgeEvent clipAtLedgeEvent) {
        if (minecraftClient.player.isSwimming() || minecraftClient.player.isInLava() || minecraftClient.player.isTouchingWater() || this.sneak.getValue().booleanValue()) {
            return;
        }
        clipAtLedgeEvent.do1162();
    }
}
