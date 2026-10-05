package me.mioclient;

import java.util.concurrent.TimeUnit;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.MoveEvent;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.event.TickEvent_2;
import me.mioclient.event.TickPostEvent;
import me.mioclient.feature.Event_3;
import me.mioclient.feature.MotionEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.client.AntiCheat;
import me.mioclient.module.movement.ElytraFly;
import net.minecraft.network.packet.s2c.play.HealthUpdateS2CPacket;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyHelper_5.class */
public class ElytraFlyHelper_5 extends ElytraFlyHelper {
    public static final AntiCheat antiCheat = (AntiCheat) BaritoneHelper_3.baritoneHelper_4.getModule117(AntiCheat.class);
    public final Stopwatch stopwatch;
    public boolean flag;
    public boolean flag2;
    public float val;

    public ElytraFlyHelper_5(ElytraFly elytraFly) {
        super(elytraFly);
        this.stopwatch = new Stopwatch();
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void onEnable() {
        this.flag = true;
        this.val = this.elytraFly.boostPitch.getValue().floatValue();
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void do27(TickEvent tickEvent) {
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void do28(MoveEvent moveEvent) {
        float floatValue = this.elytraFly.limit.getValue().floatValue();
        if (floatValue > Float.intBitsToFloat(1128792064) && is2168()) {
            floatValue = Float.intBitsToFloat(1128792064);
        }
        if (!this.elytraFly.limit.is2328() || is2168()) {
            Vec3d vec3d689 = moveEvent.getVec3d689();
            if (Math.hypot(vec3d689.getX(), vec3d689.getZ()) * Double.longBitsToDouble(4626322717216342016L) * Double.longBitsToDouble(4615288898129284301L) <= floatValue) {
                return;
            }
            Vec3d multiply = moveEvent.getVec3d689().normalize().multiply((floatValue / Double.longBitsToDouble(4615288898129284301L)) / Double.longBitsToDouble(4626322717216342016L));
            moveEvent.do691(multiply.getX(), multiply.getZ());
        }
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void do29(ChannelRead0Event channelRead0Event) {
        HealthUpdateS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof HealthUpdateS2CPacket ? (HealthUpdateS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet904 instanceof HealthUpdateS2CPacket) {
            HealthUpdateS2CPacket healthUpdateS2CPacket = packet904;
            if (healthUpdateS2CPacket.getHealth() + healthUpdateS2CPacket.getSaturation() < SearchHelper_3.get643()) {
                onDisable();
                onEnable();
            }
        }
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void do31(MotionEvent motionEvent) {
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void do32(TickPostEvent tickPostEvent) {
        this.flag2 = false;
        if (HoleSnapSearchHelper4.is2006(minecraftClient.player)) {
            return;
        }
        if (antiCheat.is238() && minecraftClient.player.isUsingItem()) {
            return;
        }
        boolean z = minecraftClient.player.input.jumping;
        if (this.elytraFly.takeoff.getValue() == ElytraFly.ElytraFlyMode_2.STRICT) {
            z = !minecraftClient.player.isOnGround() && minecraftClient.player.getVelocity().getY() < Double.longBitsToDouble(4591870180066957722L);
        } else {
            BaritoneHelper_3.holeSnapSearchHelper4_2.do2017(this.elytraFly);
        }
        if (minecraftClient.player.isFallFlying()) {
            this.stopwatch.reset();
        }
        if ((z || !this.stopwatch.is419(1500L)) && !minecraftClient.player.isFallFlying() && !SearchHelper_3.is647(minecraftClient.player) && this.elytraFly.is606()) {
            AutoSignSearchHelper4.do948();
            minecraftClient.player.startFallFlying();
        }
        if (this.elytraFly.is955()) {
            if (this.elytraFly.autoBoost.getValue().booleanValue()) {
                do2169();
            }
            float radians = (float) Math.toRadians(minecraftClient.player.getYaw());
            float floatValue = this.elytraFly.minBoost.getValue().floatValue() * Float.intBitsToFloat(1036831949);
            if (this.elytraFly.minBoost.is2328() && antiCheat.is238()) {
                floatValue = minecraftClient.player.getPitch() > 0.0f ? Float.intBitsToFloat(1020054733) : Float.intBitsToFloat(1016296636) + (this.elytraFly.verticalBoost.getValue().intValue() * Float.intBitsToFloat(981668463));
                if (minecraftClient.player.getPitch() <= 0.0f) {
                    if (!BaritoneHelper_3.holeSnapSearchHelper4_4.getStopwatch2615().is418(Double.longBitsToDouble(4621819117588971520L), TimeUnit.SECONDS)) {
                        floatValue = Float.intBitsToFloat(1006834287);
                    }
                }
            }
            if (is2168()) {
                floatValue = Float.intBitsToFloat(1077936128);
            }
            do2167();
            if (this.flag2) {
                return;
            }
            if (this.elytraFly.inLava.getValue().booleanValue() && minecraftClient.player.isInLava()) {
                floatValue = Float.intBitsToFloat(1045220557) * this.elytraFly.speed.getValue().floatValue();
            }
            if (!this.elytraFly.factorize.getValue().booleanValue() ? !(minecraftClient.options.forwardKey.isPressed() || this.elytraFly.always.getValue().booleanValue()) : minecraftClient.player.getPitch() <= 0.0f) {
                minecraftClient.player.addVelocity(MathHelper.sin(radians) * (-floatValue), 0.0d, MathHelper.cos(radians) * floatValue);
            } else if (minecraftClient.options.backKey.isPressed()) {
                minecraftClient.player.addVelocity(MathHelper.sin(radians) * floatValue, 0.0d, MathHelper.cos(radians) * (-floatValue));
            }
        }
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void do33(Event_3 event_3) {
    }

    @Override // me.mioclient.ElytraFlyHelper
    public void do329(TickEvent_2 tickEvent_2) {
        if (this.elytraFly.autoJump.getValue().booleanValue() && minecraftClient.player.isFallFlying()) {
            tickEvent_2.getInput806().jumping = true;
        }
    }

    public void do2167() {
        if (minecraftClient.player.getPitch() >= 0.0f || !this.elytraFly.nCPBoost.getValue().booleanValue()) {
            return;
        }
        int fallFlyingTicks = minecraftClient.player.getFallFlyingTicks();
        if (fallFlyingTicks % 6 > 2 || (fallFlyingTicks >= 1 && fallFlyingTicks < 10)) {
            this.flag2 = true;
        } else {
            BaritoneHelper_3.searchHelper4_8.do2478(new float[]{minecraftClient.player.getYaw(), 0.0f}, 10000, true);
            BaritoneHelper_3.searchHelper4_8.do2479();
        }
    }

    public boolean is2168() {
        return this.elytraFly.nCPBoost.getValue().booleanValue() && minecraftClient.player.getFallFlyingTicks() <= 5;
    }

    public void do2169() {
        float floatValue = this.elytraFly.boostPitch.getValue().floatValue();
        if (minecraftClient.player.getY() >= this.elytraFly.minY.getValue().intValue() + this.elytraFly.add.getValue().intValue()) {
            this.flag = true;
        }
        if (minecraftClient.player.getY() <= this.elytraFly.minY.getValue().intValue() && this.flag) {
            this.flag = false;
        }
        if (this.flag) {
            this.val += Float.intBitsToFloat(1082130432);
        } else {
            this.val -= Float.intBitsToFloat(1082130432);
        }
        this.val = MathHelper.clamp(this.val, -floatValue, Math.max(floatValue, minecraftClient.player.getPitch()));
    }

    public float get752() {
        return this.val;
    }
}
