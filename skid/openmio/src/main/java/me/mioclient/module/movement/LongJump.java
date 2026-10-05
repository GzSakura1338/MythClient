package me.mioclient.module.movement;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingHelper;
import me.mioclient.Helper_7;
import me.mioclient.HoleSnapSearchHelper4;
import me.mioclient.HoleSnapSearchHelper4_3;
import me.mioclient.KeyPearlMode;
import me.mioclient.MixinLivingEntityHelper_2;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper_2;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.MoveEvent;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.Module;
import net.minecraft.util.math.Box;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/movement/LongJump.class */
public class LongJump extends Module {
    public Setting<Float> jumpSpeed;
    public Setting<Boolean> inLiquid;
    public Setting<Boolean> useTimer;
    public Setting<Boolean> autoDisable;
    public double val;
    public double val2;
    public int num;
    public boolean flag;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/movement/LongJump$Mode.class */
    public enum Mode implements EnumSettingHelper {
        PLAIN("Plain"),
        GRIMHOP("GrimHop");

        public final String name;

        Mode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public LongJump() {
        super("LongJump", "Makes you jump fast 'n far.", Category.MOVEMENT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.num = 4;
    }

    /* JADX WARN: Code restructure failed: missing block: B:53:0x0292, code lost:
    
        if (me.mioclient.module.movement.LongJump.minecraftClient.world.isSpaceEmpty(me.mioclient.module.movement.LongJump.minecraftClient.player.getBoundingBox().offset(0.0d, me.mioclient.module.movement.LongJump.minecraftClient.player.getVelocity().y, 0.0d)) == false) goto L49;
     */
    @Listen
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    public void do242(MoveEvent moveEvent) {
        double d;
        double d2;
        if (is1815() || is1814() || minecraftClient.player.isFallFlying() || minecraftClient.player.isSpectator()) {
            return;
        }
        double d3 = moveEvent.get692();
        if (this.useTimer.getValue().booleanValue() && BaritoneHelper_3.holeSnapSearchHelper4_4.getStopwatch2615().is419(250L)) {
            BaritoneHelper_3.holeSnapSearchHelper4_2.do2018(this, Float.intBitsToFloat(1066098124));
        } else if (this.useTimer.getValue().booleanValue()) {
            BaritoneHelper_3.holeSnapSearchHelper4_2.do2017(this);
        }
        if (!BaritoneHelper_3.holeSnapSearchHelper4_4.getStopwatch2615().is419(250L) && this.autoDisable.getValue().booleanValue()) {
            do496();
        }
        double intBitsToFloat = Float.intBitsToFloat(1073741824) * this.jumpSpeed.getValue().floatValue();
        if (this.num == 1 && HoleSnapSearchHelper4_3.is2181()) {
            this.val = (Double.longBitsToDouble(4608758678669597082L) * HoleSnapSearchHelper4_3.get2512(false, Double.longBitsToDouble(4598847156609680094L) * intBitsToFloat)) - Double.longBitsToDouble(4576918229304087675L);
        } else if (this.num == 2 && HoleSnapSearchHelper4_3.is2181()) {
            d3 = Double.longBitsToDouble(4601237667291888353L) + HoleSnapSearchHelper4_3.get2513();
            this.val *= this.flag ? Double.longBitsToDouble(4610260629145325142L) : Double.longBitsToDouble(4608961340652828754L);
        } else if (this.num == 3) {
            this.val = this.val2 - (Double.longBitsToDouble(4604119971289628672L) * (this.val2 - HoleSnapSearchHelper4_3.get2512(true, Double.longBitsToDouble(4598847156609680094L) * intBitsToFloat)));
            this.flag = !this.flag;
        } else {
            if (!minecraftClient.player.verticalCollision) {
            }
            if (this.num > 0) {
                this.num = HoleSnapSearchHelper4_3.is2181() ? 1 : 0;
            }
            this.val = this.val2 - (this.val2 / Double.longBitsToDouble(4639798331726364672L));
        }
        this.val = Math.max(this.val, HoleSnapSearchHelper4_3.get2512(false, Double.longBitsToDouble(4598847156609680094L) * intBitsToFloat));
        if (HoleSnapSearchHelper4_3.is2181()) {
            double[] doubleArray2508 = HoleSnapSearchHelper4_3.getDoubleArray2508(minecraftClient.player.getYaw(SearchHelper_2.get536()), minecraftClient.player.input, this.val);
            d = doubleArray2508[0];
            d2 = doubleArray2508[1];
        } else {
            d = 0.0d;
            d2 = 0.0d;
        }
        MixinLivingEntityHelper_2.do2581(moveEvent.getVec3d689(), d, d3, d2);
        if (HoleSnapSearchHelper4_3.is2181()) {
            this.num++;
        }
    }

    @Listen(get219= Helper_7.num2)
    public void do388(MotionEvent motionEvent) {
        if (motionEvent.getKeyPearlMode1472() == KeyPearlMode.Post) {
            return;
        }
        if (!HoleSnapSearchHelper4_3.is2181()) {
            this.num = 4;
        }
        double x = minecraftClient.player.getX() - minecraftClient.player.prevX;
        double z = minecraftClient.player.getZ() - minecraftClient.player.prevZ;
        this.val2 = Math.sqrt((x * x) + (z * z));
    }

    public boolean is1814() {
        return HoleSnapSearchHelper4.is2007(minecraftClient.player) && !this.inLiquid.getValue().booleanValue();
    }

    public boolean is1815() {
        Box boundingBox = minecraftClient.player.getBoundingBox();
        return minecraftClient.world.canCollide(minecraftClient.player, new Box(minecraftClient.player.getBlockPos().getX(), boundingBox.minY, minecraftClient.player.getBlockPos().getZ(), minecraftClient.player.getBlockPos().getX() + Double.longBitsToDouble(4607182418800017408L), boundingBox.maxY, minecraftClient.player.getBlockPos().getZ() + Double.longBitsToDouble(4607182418800017408L)).contract(Double.longBitsToDouble(4502148214488346440L)));
    }
}
