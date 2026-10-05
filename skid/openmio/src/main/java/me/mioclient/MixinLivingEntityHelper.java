package me.mioclient;

import me.mioclient.mixin.ducks.DuckLivingEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MixinLivingEntityHelper.class */
public class MixinLivingEntityHelper implements SearchHelper_4 {
    public static boolean flag;

    public static void do869() {
        flag = true;
        int itemUseTimeLeft = minecraftClient.player.getItemUseTimeLeft();
        float f = minecraftClient.player.distanceTraveled;
        float speed = minecraftClient.player.limbAnimator.getSpeed();
        float f2 = minecraftClient.player.horizontalSpeed;
        float f3 = minecraftClient.player.prevHorizontalSpeed;
        float f4 = minecraftClient.player.renderYaw;
        float f5 = minecraftClient.player.lastRenderYaw;
        float f6 = minecraftClient.player.renderPitch;
        float f7 = minecraftClient.player.lastRenderPitch;
        int i = minecraftClient.player.lastAttackedTicks;
        int i2 = minecraftClient.player.handSwingTicks;
        float f8 = minecraftClient.player.lastHandSwingProgress;
        float f9 = minecraftClient.player.handSwingProgress;
        ((Helper_17)(Object) minecraftClient.player).superTick();
        ((Helper_17)(Object) minecraftClient.player).resetEvent();
        minecraftClient.player.lastAttackedTicks = i;
        ((DuckLivingEntity)(Object) minecraftClient.player).setItemUseTimeLeft(itemUseTimeLeft);
        minecraftClient.player.distanceTraveled = f;
        minecraftClient.player.limbAnimator.setSpeed(speed);
        minecraftClient.player.horizontalSpeed = f2;
        minecraftClient.player.prevHorizontalSpeed = f3;
        minecraftClient.player.renderYaw = f4;
        minecraftClient.player.lastRenderYaw = f5;
        minecraftClient.player.renderPitch = f6;
        minecraftClient.player.lastRenderPitch = f7;
        minecraftClient.player.handSwingTicks = i2;
        minecraftClient.player.lastHandSwingProgress = f8;
        minecraftClient.player.handSwingProgress = f9;
        ((Helper_17)(Object) minecraftClient.player).sendMovementPacketsWrapper();
        ((Helper_17)(Object) minecraftClient.player).resetRotations();
        flag = false;
    }

    public static boolean is870() {
        return flag;
    }
}
