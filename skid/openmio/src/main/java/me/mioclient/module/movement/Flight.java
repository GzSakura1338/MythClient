package me.mioclient.module.movement;

import me.mioclient.EnumSettingHelper;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.HoleSnapSearchHelper4_3;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper_4;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.event.MoveEvent;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.Module;
import net.minecraft.network.packet.s2c.play.PlayerPositionLookS2CPacket;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/Flight.class */
public class Flight extends Module {
    public Setting<FlightMode_2> mode;
    public Setting<Float> speed;
    public Setting<Boolean> vertical;
    public Setting<Float> vSpeed;
    public Setting<Float> glide;
    public Setting<FlightMode> antiKick;
    public Setting<Boolean> accelerate;
    public Setting<Float> accelMin;
    public Setting<Float> accelTime;
    public long num;

    /* JADX WARN: Failed to restore enum class, 'enum' modifier and super class removed */
    /* JADX WARN: Unknown enum class pattern. Please report as an issue! */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/movement/Flight$FlightMode.class */
    private enum FlightMode implements EnumSettingHelper {
        NONE("None"),
        flightMode("Plain") {
            public long num;

            @Override
            public void do388(MotionEvent motionEvent) {
                if (SearchHelper_4.minecraftClient.player.isOnGround() || System.currentTimeMillis() - this.num < 500 || SearchHelper_4.minecraftClient.player.isOnGround()) {
                    return;
                }
                this.num = System.currentTimeMillis();
                motionEvent.setY(motionEvent.get692() - 0.04d);
            }
        },
        flightMode2("Alternative") {
            public boolean flag;

            @Override
            public void do28(MoveEvent moveEvent) {
                if (SearchHelper_4.minecraftClient.player.isOnGround()) {
                    return;
                }
                double d = 0.04d;
                if (this.flag) {
                    d = -0.04d;
                }
                moveEvent.setY(moveEvent.get692() + d);
                SearchHelper_4.minecraftClient.player.setVelocity(SearchHelper_4.minecraftClient.player.getVelocity().add(0.0d, d, 0.0d));
                this.flag = !this.flag;
            }
        };

        public final String name;

        FlightMode(String str2) {
            this.name = str2;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public void do28(MoveEvent moveEvent) {
        }

        public void do388(MotionEvent motionEvent) {
        }
    }

    /* JADX WARN: Failed to restore enum class, 'enum' modifier and super class removed */
    /* JADX WARN: Unknown enum class pattern. Please report as an issue! */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/movement/Flight$FlightMode_2.class */
    public enum FlightMode_2 implements EnumSettingHelper {
        flightMode_2("Vanilla") {
            @Override
            public void do1724(Flight flight, MoveEvent moveEvent) {
                SearchHelper_4.minecraftClient.player.getAbilities().flying = true;
                SearchHelper_4.minecraftClient.player.getAbilities().setFlySpeed((float) (flight.get2634() * 0.05000000074505806d));
            }
        },
        flightMode_22("Static") {
            public int num;

            @Override
            public void do1724(Flight flight, MoveEvent moveEvent) {
                double floatValue = flight.vertical.getValue().booleanValue() ? flight.vSpeed.getValue().floatValue() : 0.0d;
                boolean z = flight.antiKick.getValue() == FlightMode.flightMode2;
                if (SearchHelper_4.minecraftClient.player.input.jumping) {
                    this.num++;
                    if (z) {
                        if (this.num >= 18) {
                            floatValue = -0.1d;
                        }
                        if (this.num >= 21) {
                            this.num = 0;
                        }
                    }
                    moveEvent.setY(floatValue);
                    SearchHelper_4.minecraftClient.player.setVelocity(SearchHelper_4.minecraftClient.player.getVelocity().withAxis(Direction.Axis.Y, floatValue));
                } else if (SearchHelper_4.minecraftClient.player.input.sneaking) {
                    moveEvent.setY(-floatValue);
                    SearchHelper_4.minecraftClient.player.setVelocity(SearchHelper_4.minecraftClient.player.getVelocity().withAxis(Direction.Axis.Y, -floatValue));
                } else {
                    moveEvent.setY(0.0d);
                    SearchHelper_4.minecraftClient.player.setVelocity(SearchHelper_4.minecraftClient.player.getVelocity().withAxis(Direction.Axis.Y, 0.0d));
                    if (!SearchHelper_4.minecraftClient.player.verticalCollision && flight.glide.getValue().floatValue() != 0.0f) {
                        SearchHelper_4.minecraftClient.player.setVelocity(SearchHelper_4.minecraftClient.player.getVelocity().add(0.0d, -flight.glide.getValue().floatValue(), 0.0d));
                        moveEvent.setY(SearchHelper_4.minecraftClient.player.getVelocity().y);
                    }
                }
                double[] doubleArray2507 = HoleSnapSearchHelper4_3.getDoubleArray2507(moveEvent, flight.get2634());
                SearchHelper_4.minecraftClient.player.setVelocity(doubleArray2507[0], SearchHelper_4.minecraftClient.player.getVelocity().y, doubleArray2507[1]);
            }
        },
        flightMode_23("JetPack") {
            @Override
            public void do1723(Flight flight, MotionEvent motionEvent) {
                if (SearchHelper_4.minecraftClient.options.jumpKey.isPressed()) {
                    SearchHelper_4.minecraftClient.player.setVelocity(SearchHelper_4.minecraftClient.player.getVelocity().add(0.0d, flight.get2634() * 0.1d, 0.0d));
                }
            }
        },
        flightMode_24("Source") {
            @Override
            public void do1724(Flight flight, MoveEvent moveEvent) {
                double d = flight.get2634();
                HoleSnapSearchHelper4_3.getDoubleArray2507(moveEvent, d);
                if (!HoleSnapSearchHelper4_3.is2181()) {
                    moveEvent.setY(0.0d);
                } else {
                    moveEvent.setY((-Math.signum(SearchHelper_4.minecraftClient.player.input.movementForward)) * Math.sin(Math.toRadians(SearchHelper_4.minecraftClient.player.getPitch())) * d);
                }
            }
        };

        public final String name;

        FlightMode_2(String str2) {
            this.name = str2;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public void do1723(Flight flight, MotionEvent motionEvent) {
        }

        public void do1724(Flight flight, MoveEvent moveEvent) {
        }
    }

    public Flight() {
        super("Flight", "Allows you to fly using magic.", Category.MOVEMENT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.speed.do2351();
        this.vSpeed.do2351();
        this.mode.do2339(() -> {
            if (isToggled()) {
                minecraftClient.player.getAbilities().flying = false;
            }
        });
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        try {
            return FontsSearchHelper4.getString1684(this.mode.getValue());
        } catch (Exception e) {
            return null;
        }
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        this.num = System.currentTimeMillis();
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        if (!is1469() && this.mode.getValue() == FlightMode_2.flightMode_2) {
            minecraftClient.player.getAbilities().flying = false;
        }
    }

    @Listen
    public void do388(MotionEvent motionEvent) {
        this.mode.getValue().do1723(this, motionEvent);
        this.antiKick.getValue().do388(motionEvent);
    }

    @Listen
    public void do28(MoveEvent moveEvent) {
        if (!HoleSnapSearchHelper4_3.is2181()) {
            this.num = System.currentTimeMillis();
        }
        this.mode.getValue().do1724(this, moveEvent);
        this.antiKick.getValue().do28(moveEvent);
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if (channelRead0Event.getPacket904() instanceof PlayerPositionLookS2CPacket) {
            this.num = System.currentTimeMillis();
        }
    }

    public double get2634() {
        return this.accelerate.getValue().booleanValue() ? HoleSnapSearchHelper4_3.get2509(this.speed.getValue().floatValue(), this.accelMin.getValue().floatValue(), this.accelTime.getValue().floatValue(), this.num) : this.speed.getValue().floatValue();
    }
}
