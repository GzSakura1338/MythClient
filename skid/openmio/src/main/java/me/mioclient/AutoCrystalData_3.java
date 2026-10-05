package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.entity.EntityType;
import net.minecraft.network.packet.s2c.play.EntitySpawnS2CPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalData_3.class */
public final class AutoCrystalData_3 {
    public final int num;
    public final EntityType<?> entityType;

    public AutoCrystalData_3(int i, EntityType<?> entityType) {
        this.num = i;
        this.entityType = entityType;
    }

    public static AutoCrystalData_3 getAutoCrystalData_32406(EntitySpawnS2CPacket entitySpawnS2CPacket) {
        return new AutoCrystalData_3(entitySpawnS2CPacket.getEntityId(), entitySpawnS2CPacket.getEntityType());
    }




    public int get391() {
        return this.num;
    }

    public EntityType<?> getEntityType2407() {
        return this.entityType;
    }
}
