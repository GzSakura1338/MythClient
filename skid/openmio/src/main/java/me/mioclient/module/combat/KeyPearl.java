package me.mioclient.module.combat;

import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FireworksHelper;
import me.mioclient.HoleSnapSearchHelper4;
import me.mioclient.KeyPearlMode;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.Module;
import me.mioclient.module.movement.ElytraFly;
import net.minecraft.item.Items;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import net.minecraft.util.Hand;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.hit.EntityHitResult;
import net.minecraft.util.hit.HitResult;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/KeyPearl.class */
public class KeyPearl extends Module {
    public static final ElytraFly elytrafly = (ElytraFly) BaritoneHelper_3.baritoneHelper_4.getModule117(ElytraFly.class);
    public Setting<Boolean> noEntityTrace;
    public Setting<Boolean> solid;
    public Setting<Boolean> disableOnElytra;
    public Setting<Boolean> disableOnRiding;

    public KeyPearl() {
        super("KeyPearl", "Throws an ender pearl.", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
        setDrawn(false);
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        if (is1469()) {
            disable();
            return;
        }
        boolean z = elytrafly.isToggled() && this.disableOnElytra.getValue().booleanValue() && !minecraftClient.player.isOnGround() && HoleSnapSearchHelper4.is2014(minecraftClient.player);
        if (minecraftClient.player.hasVehicle() && this.disableOnRiding.getValue().booleanValue()) {
            z = true;
        }
        if (z) {
            disable();
        }
    }

    @Listen
    public void do31(MotionEvent motionEvent) {
        if (motionEvent.getKeyPearlMode1472() != KeyPearlMode.Pre) {
            return;
        }
        int i = FireworksHelper.get447(Items.ENDER_PEARL);
        int i2 = FireworksHelper.get443(Items.ENDER_PEARL);
        int i3 = minecraftClient.player.getInventory().selectedSlot;
        boolean z = (minecraftClient.crosshairTarget instanceof EntityHitResult) && this.noEntityTrace.getValue().booleanValue();
        if (i2 == -1 || z || minecraftClient.player.isFallFlying() || elytrafly.is956() || minecraftClient.player.hasVehicle()) {
            disable();
            return;
        }
        BlockHitResult blockHitResult = (minecraftClient.crosshairTarget) instanceof BlockHitResult ? (BlockHitResult) (minecraftClient.crosshairTarget) : null;
        if (blockHitResult instanceof BlockHitResult) {
            BlockHitResult blockHitResult2 = blockHitResult;
            if (!minecraftClient.world.getBlockState(blockHitResult2.getBlockPos()).isReplaceable() && blockHitResult2.getType() != HitResult.Type.MISS && !this.solid.getValue().booleanValue()) {
                disable();
                return;
            }
        }
        if (Offhand.is929()) {
            return;
        }
        boolean z2 = i == -1;
        do140(z2 ? i2 : i, z2);
        minecraftClient.player.networkHandler.sendPacket(new PlayerMoveC2SPacket.Full(minecraftClient.player.getX(), minecraftClient.player.getY(), minecraftClient.player.getZ(), minecraftClient.player.getYaw(), minecraftClient.player.getPitch(), minecraftClient.player.isOnGround()));
        AutoSignSearchHelper4.do2557(Hand.MAIN_HAND);
        AutoSignSearchHelper4.do2559(Hand.MAIN_HAND);
        do140(z2 ? i2 : i3, z2);
        BaritoneHelper_3.keyPearlSearchHelper4.getList114().stream().filter(delay -> {
            return delay.setting11.getValue().booleanValue();
        }).forEach((v0) -> {
            v0.disable();
        });
        disable();
    }

    public void do140(int i, boolean z) {
        if (z) {
            FireworksHelper.do439(i);
        } else {
            FireworksHelper.do456(i);
        }
    }
}
