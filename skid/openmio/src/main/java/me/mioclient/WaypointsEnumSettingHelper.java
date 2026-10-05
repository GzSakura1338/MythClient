package me.mioclient;

import com.google.gson.annotations.SerializedName;
import net.minecraft.text.HoverEvent;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/WaypointsEnumSettingHelper.class */
public class WaypointsEnumSettingHelper implements EnumSettingHelper {

    @SerializedName("x")
    public final double val;

    @SerializedName("y")
    public final double val2;

    @SerializedName("z")
    public final double val3;

    @SerializedName("name")
    public final String name;

    @SerializedName("dimension")
    public final String string;

    @SerializedName("server")
    public final String string2;

    @SerializedName("toggled")
    public boolean toggled;
    public transient Vec3d vec3d;

    public WaypointsEnumSettingHelper(String str, double d, double d2, double d3, String str2, String str3) {
        this.val = get87(d);
        this.val2 = get87(d2);
        this.val3 = get87(d3);
        this.string = str2;
        this.string2 = str3;
        this.name = str;
        this.vec3d = new Vec3d(d, d2, d3);
        this.toggled = true;
    }

    public WaypointsEnumSettingHelper(String str, Vec3d vec3d, String str2, String str3) {
        this(str, vec3d.x, vec3d.y, vec3d.z, str2, str3);
    }

    public double get515() {
        return this.val;
    }

    public double get692() {
        return this.val2;
    }

    public double get516() {
        return this.val3;
    }

    public String getString517() {
        return this.string;
    }

    public String getString518() {
        return this.string2;
    }

    public double get3067(Vec3d vec3d) {
        return vec3d.distanceTo(this.vec3d);
    }

    public double get87(double d) {
        return PingSpoofHelper.get368(d, 1);
    }

    public boolean isToggled() {
        return this.toggled;
    }

    public void do495(boolean z) {
        this.toggled = z;
    }

    public Vec3d getVec3d1303() {
        if (this.vec3d == null) {
            this.vec3d = new Vec3d(this.val, this.val2, this.val3);
        }
        return this.vec3d;
    }

    public boolean is3068(String str, String str2) {
        return getName().equals(str) && getString518().equalsIgnoreCase(str2);
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    @Override // me.mioclient.EnumSettingHelper
    public Text getText1879() {
        return Text.literal(this.name).styled(style -> {
            return style.withHoverEvent(new HoverEvent(HoverEvent.Action.SHOW_TEXT, Text.literal(String.format("%s's info:\nx: %.1f\ny: %.1f\nz: %.1f\ndimension: %s\nserver: %s", this.name, Double.valueOf(this.val), Double.valueOf(this.val2), Double.valueOf(this.val3), this.string, this.string2)).styled(style2 -> {
                return style2.withFormatting(Formatting.GRAY);
            })));
        });
    }
}
