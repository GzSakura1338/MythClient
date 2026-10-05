package me.mioclient;

import me.mioclient.module.player.SpeedMine;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpeedMineMode.class */
public enum SpeedMineMode implements EnumSettingHelper {
    PLAIN("Plain") { // from class: me.mioclient.SpeedMineMode.Inner_2
        @Override // me.mioclient.SpeedMineMode
        public Box getBox809(SpeedMine speedMine, Box box, float f) {
            return box;
        }
    },
    IN("In") { // from class: me.mioclient.SpeedMineMode.Inner_3
        @Override // me.mioclient.SpeedMineMode
        public Box getBox809(SpeedMine speedMine, Box box, float f) {
            float f2 = 1.0f - f;
            double abs = Math.abs(box.getLengthX());
            double abs2 = Math.abs(box.getLengthY());
            double abs3 = Math.abs(box.getLengthZ());
            Vec3d center = box.getCenter();
            return new Box(center.subtract((f2 * abs) / 2.0d, (f2 * abs2) / 2.0d, (f2 * abs3) / 2.0d), center.add((f2 * abs) / 2.0d, (f2 * abs2) / 2.0d, (f2 * abs3) / 2.0d));
        }
    },
    OUT("Out") { // from class: me.mioclient.SpeedMineMode.Inner
        @Override // me.mioclient.SpeedMineMode
        public Box getBox809(SpeedMine speedMine, Box box, float f) {
            double abs = Math.abs(box.getLengthX());
            double abs2 = Math.abs(box.getLengthY());
            double abs3 = Math.abs(box.getLengthZ());
            Vec3d center = box.getCenter();
            return new Box(center.subtract((f * abs) / 2.0d, (f * abs2) / 2.0d, (f * abs3) / 2.0d), center.add((f * abs) / 2.0d, (f * abs2) / 2.0d, (f * abs3) / 2.0d));
        }
    };

    public final String name;

    SpeedMineMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public Box getBox809(SpeedMine speedMine, Box box, float f) {
        return null;
    }
}
