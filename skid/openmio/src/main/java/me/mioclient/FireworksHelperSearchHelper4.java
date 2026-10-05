package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.SendInternalEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.event.UpdateSelectedSlotEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.exploit.XCarry;
import net.minecraft.client.gui.screen.ingame.HandledScreen;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.c2s.play.ClickSlotC2SPacket;
import net.minecraft.network.packet.c2s.play.CloseHandledScreenC2SPacket;
import net.minecraft.network.packet.c2s.play.UpdateSelectedSlotC2SPacket;
import net.minecraft.network.packet.s2c.common.DisconnectS2CPacket;
import net.minecraft.network.packet.s2c.play.CloseScreenS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityStatusS2CPacket;
import net.minecraft.network.packet.s2c.play.InventoryS2CPacket;
import net.minecraft.network.packet.s2c.play.ScreenHandlerSlotUpdateS2CPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/FireworksHelperSearchHelper4.class */
public final class FireworksHelperSearchHelper4 implements SearchHelper_4 {
    public static final XCarry xCarry = (XCarry) BaritoneHelper_3.baritoneHelper_4.getModule117(XCarry.class);
    public volatile boolean flag;
    public volatile boolean flag2;
    public volatile boolean flag3;
    public boolean flag4;
    public final Map<Integer, Record> map = Collections.synchronizedMap(new HashMap());
    public final Stopwatch stopwatch = new Stopwatch();
    public int num = -1;

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/FireworksHelperSearchHelper4$Record.class */
    public static final class Record {
        public final ScreenHandlerSlotUpdateS2CPacket screenHandlerSlotUpdateS2CPacket;
        public final long num;

        public Record(ScreenHandlerSlotUpdateS2CPacket screenHandlerSlotUpdateS2CPacket, long j) {
            this.screenHandlerSlotUpdateS2CPacket = screenHandlerSlotUpdateS2CPacket;
            this.num = j;
        }




        public ScreenHandlerSlotUpdateS2CPacket getScreenHandlerSlotUpdateS2CPacket1571() {
            return this.screenHandlerSlotUpdateS2CPacket;
        }

        public long get1572() {
            return this.num;
        }
    }

    public FireworksHelperSearchHelper4() {
        baritoneHelper.do1796(this);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        synchronized (this.map) {
            ArrayList arrayList = new ArrayList();
            this.map.forEach((num, record) -> {
                if (System.currentTimeMillis() < record.get1572()) {
                    return;
                }
                arrayList.add(num);
            });
            Iterator it = arrayList.iterator();
            while (it.hasNext()) {
                Record remove = this.map.remove((Integer) it.next());
                minecraftClient.player.networkHandler.onScreenHandlerSlotUpdate(remove.getScreenHandlerSlotUpdateS2CPacket1571());
            }
        }
        this.flag4 = false;
        if (xCarry.isToggled() || (minecraftClient.currentScreen instanceof HandledScreen) || !minecraftClient.player.currentScreenHandler.getCursorStack().isEmpty() || !this.stopwatch.is419(50L) || !this.flag || System.currentTimeMillis() - BaritoneHelper_3.holeSnapSearchHelper4_4.get2618() > 2500) {
            return;
        }
        close();
    }

    @Listen(get219= Helper_7.num4)
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        if (sendImmediatelyEvent.is2403()) {
            return;
        }
        if (sendImmediatelyEvent.getPacket904() instanceof ClickSlotC2SPacket) {
            this.flag = true;
            this.stopwatch.reset();
            if (!this.flag2 || !this.flag3) {
                baritoneHelper.getObject1794(new KeyPearlModeEvent(KeyPearlMode.Pre));
            }
        }
        if ((sendImmediatelyEvent.getPacket904() instanceof CloseHandledScreenC2SPacket) || (sendImmediatelyEvent.getPacket904() instanceof DisconnectS2CPacket)) {
            this.flag = false;
        }
        UpdateSelectedSlotC2SPacket packet904 = (sendImmediatelyEvent.getPacket904()) instanceof UpdateSelectedSlotC2SPacket ? (UpdateSelectedSlotC2SPacket) (sendImmediatelyEvent.getPacket904()) : null;
        if (packet904 instanceof UpdateSelectedSlotC2SPacket) {
            UpdateSelectedSlotC2SPacket updateSelectedSlotC2SPacket = packet904;
            if (updateSelectedSlotC2SPacket.getSelectedSlot() != this.num) {
                baritoneHelper.getObject1794(new SpeedMineEvent(updateSelectedSlotC2SPacket.getSelectedSlot()));
                this.num = updateSelectedSlotC2SPacket.getSelectedSlot();
            } else if (SearchHelper4_8.is724()) {
                sendImmediatelyEvent.do1162();
            }
        }
        if (sendImmediatelyEvent.getPacket904() instanceof DisconnectS2CPacket) {
            this.num = -1;
        }
    }

    @Listen(get219= Helper_7.num2)
    public void do29(ChannelRead0Event channelRead0Event) {
        if (channelRead0Event.getPacket904() instanceof CloseScreenS2CPacket) {
            this.flag = false;
        }
        if (channelRead0Event.getPacket904() instanceof InventoryS2CPacket) {
            this.map.clear();
        }
        ScreenHandlerSlotUpdateS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof ScreenHandlerSlotUpdateS2CPacket ? (ScreenHandlerSlotUpdateS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet904 instanceof ScreenHandlerSlotUpdateS2CPacket) {
            ScreenHandlerSlotUpdateS2CPacket screenHandlerSlotUpdateS2CPacket = packet904;
            if (SearchHelper4_8.is724()) {
                if (screenHandlerSlotUpdateS2CPacket.getSlot() == 36 + minecraftClient.player.getInventory().selectedSlot || screenHandlerSlotUpdateS2CPacket.getSlot() < 36 || screenHandlerSlotUpdateS2CPacket.getSlot() > 44) {
                    return;
                }
                if (!is2629(screenHandlerSlotUpdateS2CPacket.getStack(), screenHandlerSlotUpdateS2CPacket.getSlot())) {
                    this.map.remove(Integer.valueOf(screenHandlerSlotUpdateS2CPacket.getSlot()));
                    return;
                } else {
                    this.map.put(Integer.valueOf(screenHandlerSlotUpdateS2CPacket.getSlot()), new Record(screenHandlerSlotUpdateS2CPacket, System.currentTimeMillis() + 500));
                    channelRead0Event.do1162();
                }
            }
        }
        EntityStatusS2CPacket packet9042 = (channelRead0Event.getPacket904()) instanceof EntityStatusS2CPacket ? (EntityStatusS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet9042 instanceof EntityStatusS2CPacket) {
            EntityStatusS2CPacket entityStatusS2CPacket = packet9042;
            if (entityStatusS2CPacket.getStatus() == 35 && entityStatusS2CPacket.getEntity(minecraftClient.world) == minecraftClient.player) {
                this.flag4 = true;
            }
        }
    }

    @Listen
    public void onSendInternal(SendInternalEvent sendInternalEvent) {
        if (sendInternalEvent.getPacket904() instanceof ClickSlotC2SPacket) {
            if (this.flag2 && this.flag3) {
                return;
            }
            baritoneHelper.getObject1794(new KeyPearlModeEvent(KeyPearlMode.Post));
            this.flag3 = true;
        }
    }

    @Listen
    public void onUpdateSelectedSlot(UpdateSelectedSlotEvent updateSelectedSlotEvent) {
        if (minecraftClient.isInSingleplayer()) {
            return;
        }
        AutoSignSearchHelper4.do2569(minecraftClient.player.getInventory().selectedSlot);
        updateSelectedSlotEvent.do1162();
    }

    public boolean is2629(ItemStack itemStack, int i) {
        for (int i2 = 0; i2 < 9; i2++) {
            if (i - 36 != i2) {
                if (ItemStack.areEqual(minecraftClient.player.getInventory().getStack(i2), itemStack)) {
                    return true;
                }
            }
        }
        return false;
    }

    public void close() {
        minecraftClient.player.networkHandler.sendPacket(new CloseHandledScreenC2SPacket(minecraftClient.player.currentScreenHandler.syncId));
        this.flag = false;
    }

    public void do2630(boolean z) {
        if (this.flag2 == z) {
            return;
        }
        this.flag2 = z;
        this.flag3 = false;
    }

    public boolean is2631() {
        return this.flag4;
    }

    public int get2632() {
        return this.num;
    }
}
