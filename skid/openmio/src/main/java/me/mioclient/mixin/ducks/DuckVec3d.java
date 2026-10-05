package me.mioclient.mixin.ducks;

import net.minecraft.util.math.Vec3d;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({Vec3d.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckVec3d.class */
public interface DuckVec3d {
    @Accessor("x")
    @Mutable
    void setX(double d);

    @Accessor("y")
    @Mutable
    void setY(double d);

    @Accessor("z")
    @Mutable
    void setZ(double d);
}
