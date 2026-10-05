package me.mioclient.module.movement;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingHelper;
import me.mioclient.Helper_7;
import me.mioclient.KeyPearlMode;
import me.mioclient.PhaseESPHelper;
import me.mioclient.WarpHelper;
import me.mioclient.WarpHelperMode;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.event.MoveEvent;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.Module;
import net.minecraft.network.packet.s2c.play.PlayerPositionLookS2CPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/Warp.class */
public class Warp extends Module {
    public static FakeLag fakeLag2 = (FakeLag) BaritoneHelper_3.baritoneHelper_4.getModule117(FakeLag.class);
    public Setting<WarpMode> mode;
    public Setting<Integer> boost;
    public Setting<Integer> charge;
    public Setting<Integer> chargeSpeed;
    public Setting<WarpHelperMode> recharge;
    public Setting<Boolean> fakeLag;
    public Setting<Boolean> disableGround;
    public Setting<Boolean> limitPackets;
    public Setting<Boolean> autoDisable;
    public final WarpHelper warpHelper;
    public boolean flag;
    public volatile int num;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/movement/Warp$WarpMode.class */
    public enum WarpMode implements EnumSettingHelper {
        PLAIN("Plain"),
        ALTERNATIVE("Alternative");

        public final String name;

        WarpMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public Warp() {
        super("Warp", "Allows you to dash forward after standing still for a certain amount of time.", Category.MOVEMENT, "tickshift");
        PhaseESPHelper.do1351(this);
        this.warpHelper = new WarpHelper(this);
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        return this.mode.getValue() == WarpMode.ALTERNATIVE ? String.valueOf(this.warpHelper.queue.size()) : String.valueOf(this.num);
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        if (this.recharge.getValue() == WarpHelperMode.INSTANT) {
            this.num = this.charge.getValue().intValue();
        }
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        do2179(false);
        if (this.mode.getValue() != WarpMode.ALTERNATIVE || is1469()) {
            return;
        }
        while (!this.warpHelper.queue.isEmpty()) {
            try {
                minecraftClient.player.networkHandler.sendPacket(this.warpHelper.queue.poll());
            } catch (Exception e) {
            }
        }
        this.warpHelper.queue.clear();
    }

    @Listen(get219= Helper_7.num5)
    public void onMove(MoveEvent moveEvent) {
        if (this.mode.getValue() != WarpMode.PLAIN) {
            return;
        }
        if (!is2180()) {
            BaritoneHelper_3.inner.do2017(this);
            do2179(false);
            this.flag = false;
            return;
        }
        if (this.num == 0 || this.flag) {
            BaritoneHelper_3.inner.do2017(this);
            if (this.autoDisable.getValue().booleanValue()) {
                do496();
            }
            do2179(false);
            return;
        }
        if (this.num > 0) {
            if (this.fakeLag.getValue().booleanValue()) {
                boolean isToggled = fakeLag2.isToggled();
                do2179(minecraftClient.player.isOnGround() || !this.disableGround.getValue().booleanValue());
                if (this.boost.getValue().intValue() > 1 && this.limitPackets.getValue().booleanValue()) {
                    BaritoneHelper_3.holeSnapSearchHelper4_4.getStopwatch2616().reset();
                }
                if (!isToggled) {
                    return;
                }
            }
            BaritoneHelper_3.inner.do2018(this, this.boost.getValue().intValue());
        }
    }

    @Listen
    public void do31(MotionEvent motionEvent) {
        if (this.mode.getValue() == WarpMode.ALTERNATIVE && motionEvent.getKeyPearlMode1472() == KeyPearlMode.Pre) {
            if (!this.warpHelper.queue.isEmpty() && is2180()) {
                BaritoneHelper_3.inner.do2018(this, this.boost.getValue().intValue());
                minecraftClient.player.networkHandler.sendPacket(this.warpHelper.queue.poll());
                return;
            }
            if (!this.warpHelper.queue.isEmpty() && !this.flag) {
                BaritoneHelper_3.inner.do2017(this);
                return;
            }
            BaritoneHelper_3.inner.do2017(this);
            if (this.autoDisable.getValue().booleanValue()) {
                do496();
            }
        }
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if (channelRead0Event.getPacket904() instanceof PlayerPositionLookS2CPacket) {
            this.flag = true;
        }
    }

    public void do2179(boolean z) {
        if (!this.fakeLag.getValue().booleanValue() || minecraftClient.player == null) {
            return;
        }
        fakeLag2.do495(z);
    }

    public boolean is2180() {
        return minecraftClient.player.input.pressingRight || minecraftClient.player.input.pressingLeft || minecraftClient.player.input.pressingBack || minecraftClient.player.input.pressingForward || minecraftClient.player.getX() - minecraftClient.player.prevX != 0.0d || minecraftClient.player.getY() - minecraftClient.player.prevY != 0.0d || minecraftClient.player.getZ() - minecraftClient.player.prevZ != 0.0d;
    }

    public int get2093() {
        return this.mode.getValue() == WarpMode.ALTERNATIVE ? this.warpHelper.queue.size() : this.num;
    }

    public boolean is2181() {
        return minecraftClient.player.getPos().squaredDistanceTo(minecraftClient.player.prevX, minecraftClient.player.prevY, minecraftClient.player.prevZ) > 0.0d;
    }
}
