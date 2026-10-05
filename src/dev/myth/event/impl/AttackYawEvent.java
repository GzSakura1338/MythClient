package dev.myth.event.impl;

import dev.myth.event.api.Event;

public class AttackYawEvent implements Event {
    private float yaw;

    public AttackYawEvent(float yaw) {
        this.yaw = yaw;
    }

    public float getYaw() {
        return yaw;
    }

    public void setYaw(float yaw) {
        this.yaw = yaw;
    }
}
