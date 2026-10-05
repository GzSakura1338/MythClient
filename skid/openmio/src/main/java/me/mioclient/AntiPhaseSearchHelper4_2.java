package me.mioclient;

import me.mioclient.event.Listen;
import me.mioclient.event.SendInternalEvent;
import me.mioclient.event.TickEvent;
import net.minecraft.network.packet.c2s.play.ClientCommandC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AntiPhaseSearchHelper4_2.class */
public final class AntiPhaseSearchHelper4_2 implements SearchHelper_4 {
    public double val;
    public double val2;
    public double val3;
    public boolean flag;
    public boolean flag2;
    public boolean flag3;
    public boolean flag4;
    public int num;
    public int num2;
    public float[] floatArr;

    public AntiPhaseSearchHelper4_2() {
        baritoneHelper.do1796(this);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        this.flag3 = this.flag2;
        this.flag2 = minecraftClient.player.isFallFlying();
        if (minecraftClient.player.isOnGround() || this.flag) {
            this.num2 = 0;
            this.num++;
        } else {
            this.num = 0;
            this.num2++;
        }
    }

    @Listen
    public void onSendInternal(SendInternalEvent sendInternalEvent) {
        PlayerMoveC2SPacket packet904 = (sendInternalEvent.getPacket904()) instanceof PlayerMoveC2SPacket ? (PlayerMoveC2SPacket) (sendInternalEvent.getPacket904()) : null;
        if (packet904 instanceof PlayerMoveC2SPacket) {
            PlayerMoveC2SPacket playerMoveC2SPacket = packet904;
            this.val = playerMoveC2SPacket.getX(minecraftClient.player.getX());
            this.val2 = playerMoveC2SPacket.getY(minecraftClient.player.getY());
            this.val3 = playerMoveC2SPacket.getZ(minecraftClient.player.getZ());
            this.flag = playerMoveC2SPacket.isOnGround();
            this.floatArr = new float[]{playerMoveC2SPacket.getYaw(minecraftClient.player.getYaw()), playerMoveC2SPacket.getPitch(minecraftClient.player.getPitch())};
        }
        ClientCommandC2SPacket packet9042 = (sendInternalEvent.getPacket904()) instanceof ClientCommandC2SPacket ? (ClientCommandC2SPacket) (sendInternalEvent.getPacket904()) : null;
        if (packet9042 instanceof ClientCommandC2SPacket) {
            ClientCommandC2SPacket clientCommandC2SPacket = packet9042;
            if (clientCommandC2SPacket.getMode() == ClientCommandC2SPacket.Mode.PRESS_SHIFT_KEY) {
                this.flag4 = true;
            } else if (clientCommandC2SPacket.getMode() == ClientCommandC2SPacket.Mode.RELEASE_SHIFT_KEY) {
                this.flag4 = false;
            }
        }
    }

    public void do2226(java.lang.Runnable runnable) {
        if (!BaritoneHelper_3.antiPhaseSearchHelper4_2.is2230()) {
            runnable.run();
            return;
        }
        AutoSignSearchHelper4.do2571(new ClientCommandC2SPacket(minecraftClient.player, ClientCommandC2SPacket.Mode.RELEASE_SHIFT_KEY));
        runnable.run();
        AutoSignSearchHelper4.do2571(new ClientCommandC2SPacket(minecraftClient.player, ClientCommandC2SPacket.Mode.PRESS_SHIFT_KEY));
    }

    public void do2227(java.lang.Runnable runnable) {
        if (!(!BaritoneHelper_3.antiPhaseSearchHelper4_2.is2230())) {
            runnable.run();
            return;
        }
        AutoSignSearchHelper4.do2571(new ClientCommandC2SPacket(minecraftClient.player, ClientCommandC2SPacket.Mode.PRESS_SHIFT_KEY));
        runnable.run();
        AutoSignSearchHelper4.do2571(new ClientCommandC2SPacket(minecraftClient.player, ClientCommandC2SPacket.Mode.RELEASE_SHIFT_KEY));
    }

    public double get515() {
        return this.val;
    }

    public double get692() {
        return this.val2;
    }

    public double get516() {
        return this.val3;
    }

    public Vec3d getVec3d1954() {
        return new Vec3d(this.val, this.val2, this.val3);
    }

    public boolean is2228() {
        return this.flag;
    }

    public boolean is2229() {
        return this.flag3;
    }

    public boolean is2230() {
        return this.flag4;
    }

    public int get2231() {
        return this.num;
    }

    public int get2232() {
        return this.num2;
    }

    public float[] getFloatArray2233() {
        return this.floatArr;
    }
}
