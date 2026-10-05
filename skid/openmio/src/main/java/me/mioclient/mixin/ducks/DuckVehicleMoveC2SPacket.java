package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.c2s.play.VehicleMoveC2SPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({VehicleMoveC2SPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckVehicleMoveC2SPacket.class */
public interface DuckVehicleMoveC2SPacket {
    @Accessor("y")
    @Mutable
    void setY(double d);
}
