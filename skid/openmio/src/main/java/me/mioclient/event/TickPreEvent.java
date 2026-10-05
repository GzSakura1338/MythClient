package me.mioclient.event;

import me.mioclient.SearchHelper_4;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/TickPreEvent.class */
public final class TickPreEvent extends MoveEvent {
    public TickPreEvent(Vec3d vec3d) {
        super(vec3d, null);
    }

    public static boolean is1301(Object obj) {
        return SearchHelper_4.minecraftClient.player != null && (obj instanceof Entity) && SearchHelper_4.minecraftClient.player.getVehicle() == obj;
    }
}
