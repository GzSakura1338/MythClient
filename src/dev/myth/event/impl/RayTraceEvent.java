package dev.myth.event.impl;

import dev.myth.event.api.Event;
import net.minecraft.world.entity.Entity;

public class RayTraceEvent implements Event {
    public Entity entity;
    public float yaw;
    public float pitch;

    public RayTraceEvent(Entity entity, float yaw, float pitch) {
        this.entity = entity;
        this.yaw = yaw;
        this.pitch = pitch;
    }

    public Entity getEntity() {
        return entity;
    }

    public void setEntity(Entity entity) {
        this.entity = entity;
    }

    public float getYaw() {
        return yaw;
    }

    public void setYaw(float yaw) {
        this.yaw = yaw;
    }

    public float getPitch() {
        return pitch;
    }

    public void setPitch(float pitch) {
        this.pitch = pitch;
    }
}
