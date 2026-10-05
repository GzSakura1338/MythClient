package me.mioclient.module.movement;

import me.mioclient.AntiCheatVelocityHelper;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ElytraFlyHelper_4;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.Helper_7;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PingSpoofHelper;
import me.mioclient.VelocityHelper;
import me.mioclient.VelocityHelper_2;
import me.mioclient.VelocityHelper_3;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.ExplosionVelocityEvent;
import me.mioclient.event.InteractBlockEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.PushOutOfBlocksEvent;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickPostEvent;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.Module;
import me.mioclient.module.client.AntiCheat;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/Velocity.class */
public class Velocity extends Module {
    public static final Sprint sprint = (Sprint) BaritoneHelper_3.baritoneHelper_4.getModule117(Sprint.class);
    public static final AntiCheat antiCheat = (AntiCheat) BaritoneHelper_3.baritoneHelper_4.getModule117(AntiCheat.class);
    public Setting<VelocityMode> mode;
    public Setting<Integer> horizontal;
    public Setting<Integer> vertical;
    public Setting<Boolean> inverse;
    public Setting<Boolean> explosions;
    public Setting<Boolean> push;
    public Setting<Boolean> liquids;
    public final ElytraFlyHelper_4<VelocityMode, VelocityHelper> elytraFlyHelper_4;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/movement/Velocity$VelocityMode.class */
    public enum VelocityMode implements EnumSettingHelper {
        PLAIN("Plain"),
        GRIM("Grim"),
        WALLS("Walls");

        public final String name;

        VelocityMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public Velocity() {
        super("Velocity", "Cancels all the pushing you receive.", Category.MOVEMENT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.elytraFlyHelper_4 = new ElytraFlyHelper_4<>(this.mode);
        ElytraFlyHelper_4 elytraFlyHelper_4 = this.elytraFlyHelper_4;
        elytraFlyHelper_4.do997(VelocityMode.PLAIN, new VelocityHelper_2(this));
        ElytraFlyHelper_4 elytraFlyHelper_42 = this.elytraFlyHelper_4;
        elytraFlyHelper_42.do997(VelocityMode.GRIM, new VelocityHelper_3(this));
        ElytraFlyHelper_4 elytraFlyHelper_43 = this.elytraFlyHelper_4;
        elytraFlyHelper_43.do997(VelocityMode.WALLS, new AntiCheatVelocityHelper(this));
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        if (this.mode.getValue() != VelocityMode.PLAIN) {
            return FontsSearchHelper4.getString1684(this.mode.getValue().getName());
        }
        return new ArgumentTypeHelper().getArgumentTypeHelper2909(PingSpoofHelper.get369(this.vertical.getValue().intValue(), 1)).getArgumentTypeHelper2909(PingSpoofHelper.get369(this.horizontal.getValue().intValue(), 1)).getString2921("H\u0001% V\u0001%");
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        sprint.flag2 = false;
        this.elytraFlyHelper_4.getObject996().onDisable();
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        this.elytraFlyHelper_4.getObject996().do30(sendImmediatelyEvent);
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        this.elytraFlyHelper_4.getObject996().do29(channelRead0Event);
    }

    @Listen(get219= Helper_7.num5)
    public void do32(TickPostEvent tickPostEvent) {
        if (this.mode.getValue() != VelocityMode.WALLS) {
            sprint.flag2 = false;
        }
        this.elytraFlyHelper_4.getObject996().do711();
    }

    @Listen
    public void onInteractBlock(InteractBlockEvent interactBlockEvent) {
        this.elytraFlyHelper_4.getObject996().onInteractBlock(interactBlockEvent);
    }

    @Listen
    public void do31(MotionEvent motionEvent) {
        this.elytraFlyHelper_4.getObject996().do31(motionEvent);
    }

    @Listen
    public void do598(ExplosionVelocityEvent explosionVelocityEvent) {
        this.elytraFlyHelper_4.getObject996().do598(explosionVelocityEvent);
    }

    @Listen
    public void onPushOutOfBlocks(PushOutOfBlocksEvent pushOutOfBlocksEvent) {
        if (this.push.getValue().booleanValue()) {
            pushOutOfBlocksEvent.do1162();
        }
    }

    public boolean is1893() {
        if (!isToggled()) {
            return false;
        }
        if ((this.elytraFlyHelper_4.getObject996() instanceof VelocityHelper_2) && this.horizontal.getValue().intValue() == 0 && this.vertical.getValue().intValue() == 0) {
            return true;
        }
        VelocityHelper object996 = this.elytraFlyHelper_4.getObject996();
        return (object996 instanceof VelocityHelper_3) && !((VelocityHelper_3) object996).is715();
    }

    public VelocityHelper getVelocityHelper1894() {
        return this.elytraFlyHelper_4.getObject996();
    }
}
