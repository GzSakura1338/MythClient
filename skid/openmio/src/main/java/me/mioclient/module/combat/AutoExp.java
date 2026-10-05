package me.mioclient.module.combat;

import java.util.Iterator;
import me.mioclient.ArmorSearchHelper4;
import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.KeyPearlMode;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper;
import me.mioclient.SearchHelper4_8;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.feature.MotionEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import me.mioclient.module.misc.AntiAim;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.Hand;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.hit.HitResult;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/AutoExp.class */
public class AutoExp extends Module {
    public Setting<SpeedMineMode> mode;
    public Setting<Integer> delay;
    public Setting<Integer> frequency;
    public Setting<Boolean> rotate;
    public Setting<Boolean> air;
    public Setting<Boolean> stop;
    public Setting<Integer> stopAt;
    public Setting<Boolean> autoDisable;
    public Setting<Boolean> tools;
    public static AntiAim antiAim = (AntiAim) BaritoneHelper_3.baritoneHelper_4.getModule117(AntiAim.class);
    public final Stopwatch stopwatch;
    public int num;
    public boolean flag;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/combat/AutoExp$SpeedMineMode.class */
    public enum SpeedMineMode implements EnumSettingHelper {
        NORMAL("Normal"),
        SILENT("Silent");

        public final String name;

        SpeedMineMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public AutoExp() {
        super("AutoExp", "Mends your armor for you.", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
        this.flag = false;
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        if (is1469()) {
            return;
        }
        if (this.rotate.getValue().booleanValue()) {
            this.stopwatch.reset();
        }
        if (antiAim.isToggled() && !this.flag) {
            antiAim.do856();
        }
        this.num = minecraftClient.player.getInventory().selectedSlot;
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        if (is1469() || minecraftClient.player.getInventory().selectedSlot == this.num || this.mode.getValue() != SpeedMineMode.NORMAL) {
            return;
        }
        FireworksHelper.do456(this.num);
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        if (this.flag) {
            return "Stopped";
        }
        return String.valueOf(minecraftClient.player.getInventory().main.stream().filter(itemStack -> {
            return itemStack.getItem() == Items.EXPERIENCE_BOTTLE;
        }).mapToInt((v0) -> {
            return v0.getCount();
        }).sum());
    }

    @Listen(get219= 250)
    public void do31(MotionEvent motionEvent) {
        if (!is130() && motionEvent.getKeyPearlMode1472() == KeyPearlMode.Pre) {
            float[] floatArray2484 = SearchHelper4_8.getFloatArray2484(SearchHelper.getVec3d222(BaritoneHelper_3.mainhandHelper_2.getBox1109(minecraftClient.player, 1)));
            if (!this.flag && this.rotate.getValue().booleanValue()) {
                BaritoneHelper_3.searchHelper4_8.do2478(floatArray2484, 4, true);
            }
            boolean isHolding = minecraftClient.player.isHolding(Items.EXPERIENCE_BOTTLE);
            int i = FireworksHelper.get447(Items.EXPERIENCE_BOTTLE);
            if (this.stop.getValue().booleanValue()) {
                boolean z = true;
                Iterator it = minecraftClient.player.getInventory().armor.iterator();
                while (true) {
                    if (!it.hasNext()) {
                        break;
                    }
                    if (is1447((ItemStack) it.next())) {
                        z = false;
                        break;
                    }
                }
                if ((is1447(minecraftClient.player.getMainHandStack()) || is1447(minecraftClient.player.getOffHandStack())) && this.tools.getValue().booleanValue()) {
                    z = false;
                }
                if (z || i == -1) {
                    this.flag = true;
                    if (this.autoDisable.getValue().booleanValue()) {
                        disable();
                        return;
                    }
                    return;
                }
            }
            if (i != -1 || isHolding) {
                if (this.stopwatch.is419(this.delay.getValue().intValue() * 50)) {
                    Hand hand = minecraftClient.player.getOffHandStack().isOf(Items.EXPERIENCE_BOTTLE) ? Hand.OFF_HAND : Hand.MAIN_HAND;
                    this.flag = false;
                    int i2 = minecraftClient.player.getInventory().selectedSlot;
                    if (minecraftClient.player.getMainHandStack().getItem() != Items.EXPERIENCE_BOTTLE && hand == Hand.MAIN_HAND) {
                        FireworksHelper.do456(i);
                    }
                    for (int i3 = 0; i3 < this.frequency.getValue().intValue(); i3++) {
                        if (this.rotate.getValue().booleanValue()) {
                            AutoSignSearchHelper4.do2558(hand, floatArray2484[0], floatArray2484[1]);
                        } else {
                            AutoSignSearchHelper4.do2557(hand);
                        }
                    }
                    if (this.mode.getValue() == SpeedMineMode.SILENT && minecraftClient.player.getInventory().selectedSlot != i2) {
                        FireworksHelper.do456(i2);
                    }
                    this.stopwatch.reset();
                }
            }
        }
    }

    public boolean is1447(ItemStack itemStack) {
        return itemStack.isDamageable() && !itemStack.isEmpty() && ArmorSearchHelper4.get1905(itemStack) < this.stopAt.getValue().intValue();
    }

    public boolean is130() {
        if (!this.air.getValue().booleanValue()) {
            if (minecraftClient.crosshairTarget.getType() == HitResult.Type.MISS) {
                return true;
            }
            BlockHitResult blockHitResult = (minecraftClient.crosshairTarget) instanceof BlockHitResult ? (BlockHitResult) (minecraftClient.crosshairTarget) : null;
            if (blockHitResult instanceof BlockHitResult) {
                BlockHitResult blockHitResult2 = blockHitResult;
                if (minecraftClient.world.getBlockState(blockHitResult2.getBlockPos()).isReplaceable()) {
                    return true;
                }
            }
        }
        if (Offhand.is929()) {
            return true;
        }
        return minecraftClient.player.isFallFlying();
    }
}
