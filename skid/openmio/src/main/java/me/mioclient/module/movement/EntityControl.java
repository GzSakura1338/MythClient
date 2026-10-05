package me.mioclient.module.movement;

import me.mioclient.EntityControlSearchHelper4;
import me.mioclient.HoleSnapSearchHelper4_3;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper_2;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.event.TickPreEvent;
import me.mioclient.module.Module;
import net.minecraft.entity.Entity;
import net.minecraft.entity.Saddleable;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.network.packet.s2c.play.PlayerPositionLookS2CPacket;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/EntityControl.class */
public class EntityControl extends Module {
    public Setting<Boolean> entitySpeed;
    public Setting<Double> speed;
    public Setting<Boolean> noCollision;
    public Setting<Boolean> flight;
    public Setting<Float> vertical;
    public Setting<Boolean> glide;
    public Setting<Boolean> accelerate;
    public Setting<Boolean> copyYaw;
    public Setting<Boolean> horseJump;
    public boolean flag;
    public long num;

    public EntityControl() {
        super("EntityControl", "Helps with riding entities.", Category.MOVEMENT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.speed.do2339(() -> {
            this.num = System.currentTimeMillis();
        });
    }

    @Listen
    public void onTickPre(TickPreEvent tickPreEvent) {
        Entity vehicle;
        double d;
        if (is1469()) {
            return;
        }
        if (!HoleSnapSearchHelper4_3.is2181()) {
            this.num = System.currentTimeMillis();
        }
        if (minecraftClient.player.hasVehicle() && (vehicle = minecraftClient.player.getVehicle()) != null) {
            if (this.copyYaw.getValue().booleanValue()) {
                vehicle.setYaw(minecraftClient.player.getYaw(SearchHelper_2.get536()));
            }
            if (!(vehicle instanceof Saddleable) || ((Saddleable) vehicle).isSaddled()) {
                double doubleValue = this.speed.getValue().doubleValue();
                double floatValue = this.vertical.getValue().floatValue();
                if (doubleValue >= Double.longBitsToDouble(4626322717216342016L)) {
                    doubleValue = Double.longBitsToDouble(4626319902466574909L);
                }
                if (this.accelerate.getValue().booleanValue()) {
                    doubleValue = HoleSnapSearchHelper4_3.get2509(this.speed.getValue().doubleValue(), Double.longBitsToDouble(4591870180066957722L), this.speed.getValue().doubleValue(), this.num);
                }
                if (!this.entitySpeed.getValue().booleanValue() || is1861()) {
                    return;
                }
                double[] doubleArray2508 = HoleSnapSearchHelper4_3.getDoubleArray2508(minecraftClient.player.getYaw(SearchHelper_2.get536()), minecraftClient.player.input, doubleValue);
                if (this.flight.getValue().booleanValue()) {
                    if (vehicle instanceof BoatEntity) {
                        ((BoatEntity) vehicle).setNoGravity(true);
                    }
                    if (minecraftClient.options.jumpKey.isPressed()) {
                        d = floatValue;
                    } else if (this.glide.getValue().booleanValue() || EntityControlSearchHelper4.is1538(minecraftClient.options.sprintKey)) {
                        d = -floatValue;
                    } else {
                        d = this.flag ? Double.longBitsToDouble(-4637266464074629120L) : Double.longBitsToDouble(4586105572780146688L);
                    }
                } else {
                    d = tickPreEvent.get692();
                }
                tickPreEvent.do690(new Vec3d(doubleArray2508[0], d, doubleArray2508[1]));
                this.flag = !this.flag;
            }
        }
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if (channelRead0Event.getPacket904() instanceof PlayerPositionLookS2CPacket) {
            this.num = System.currentTimeMillis();
        }
    }

    public boolean is1860(Object obj) {
        return false;
    }

    public boolean is1861() {
        return minecraftClient.player.hasVehicle() && minecraftClient.player.getVehicle().horizontalCollision && this.noCollision.getValue().booleanValue();
    }
}
