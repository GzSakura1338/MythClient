package me.mioclient.module.movement;

import java.awt.Color;
import java.util.ArrayDeque;
import java.util.Queue;
import java.util.concurrent.TimeUnit;
import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.EnumSettingHelper;
import me.mioclient.Helper_7;
import me.mioclient.MatrixStackEvent;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.DisconnectEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.c2s.common.CommonPongC2SPacket;
import net.minecraft.network.packet.c2s.play.HandSwingC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerActionC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractBlockC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import net.minecraft.network.packet.c2s.play.UpdateSelectedSlotC2SPacket;
import net.minecraft.network.packet.s2c.play.PlayerPositionLookS2CPacket;
import net.minecraft.util.math.Box;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/FakeLag.class */
public class FakeLag extends Module {
    public final Queue<Packet<?>> queue;
    public Setting<FakeLagMode> mode;
    public Setting<Integer> timeout;
    public Setting<Boolean> packets;
    public Setting<Boolean> movement;
    public Setting<Boolean> interact;
    public Setting<Boolean> swing;
    public Setting<Boolean> swap;
    public Setting<Boolean> interactEntity;
    public Setting<Boolean> digging;
    public Setting<Boolean> all;
    public Setting<Boolean> autoDisable;
    public Setting<Float> disableTime;
    public Setting<Boolean> render;
    public Setting<Color> fill;
    public Setting<Color> outline;
    public Setting<Float> lineWidth;
    public final Stopwatch stopwatch;
    public final Stopwatch stopwatch2;
    public Box box;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/movement/FakeLag$FakeLagMode.class */
    public enum FakeLagMode implements EnumSettingHelper {
        BLINK("Blink"),
        PULSE("Pulse");

        public final String name;

        FakeLagMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public FakeLag() {
        super("FakeLag", "Cancels movement packets until toggled off.", Category.MOVEMENT, new String[0]);
        this.queue = new ArrayDeque();
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
        this.stopwatch2 = new Stopwatch();
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        this.queue.clear();
        if (is1469()) {
            disable();
            return;
        }
        this.box = minecraftClient.player.getBoundingBox();
        this.stopwatch.reset();
        this.stopwatch2.reset();
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        if (is1469()) {
            return;
        }
        do604();
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (this.mode.getValue() == FakeLagMode.PULSE) {
            if (this.stopwatch2.is419(this.timeout.getValue().intValue())) {
                do604();
                this.stopwatch2.reset();
            }
        }
        if (this.autoDisable.getValue().booleanValue()) {
            if (this.stopwatch.is418(this.disableTime.getValue().floatValue(), TimeUnit.SECONDS)) {
                do496();
            }
        }
    }

    @Listen(get219= Helper_7.num5)
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        if (!is1826(sendImmediatelyEvent.getPacket904()) || sendImmediatelyEvent.is2403()) {
            return;
        }
        this.queue.add(sendImmediatelyEvent.getPacket904());
        sendImmediatelyEvent.do1162();
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if ((channelRead0Event.getPacket904() instanceof PlayerPositionLookS2CPacket) && this.mode.getValue() == FakeLagMode.PULSE) {
            this.stopwatch2.setTime(-1L);
        }
    }

    @Listen
    public void onEvent(MatrixStackEvent.Inner_3 inner_3) {
        if (this.render.getValue().booleanValue()) {
            PhaseESPSearchHelper4.do1590(inner_3.getMatrixStack472(), this.box, this.fill.getValue());
            PhaseESPSearchHelper4.do1593(inner_3.getMatrixStack472(), this.box, this.outline.getValue(), this.lineWidth.getValue().floatValue());
        }
    }

    @Listen
    public void onDisconnect(DisconnectEvent disconnectEvent) {
        disable();
    }

    public boolean is1826(Packet<?> packet) {
        return ((packet instanceof PlayerMoveC2SPacket) && this.movement.getValue().booleanValue()) || ((packet instanceof PlayerInteractBlockC2SPacket) && this.interact.getValue().booleanValue()) || (((packet instanceof PlayerInteractEntityC2SPacket) && this.interactEntity.getValue().booleanValue()) || (((packet instanceof PlayerActionC2SPacket) && this.digging.getValue().booleanValue()) || (((packet instanceof HandSwingC2SPacket) && this.swing.getValue().booleanValue()) || (((packet instanceof UpdateSelectedSlotC2SPacket) && this.swap.getValue().booleanValue()) || (packet instanceof CommonPongC2SPacket) || this.all.getValue().booleanValue()))));
    }

    public void do604() {
        while (!this.queue.isEmpty()) {
            AutoSignSearchHelper4.do2573(this.queue.poll());
        }
        this.box = minecraftClient.player.getBoundingBox();
    }
}
