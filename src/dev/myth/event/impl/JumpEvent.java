package dev.myth.event.impl;

import dev.myth.event.api.Event;

public class JumpEvent implements Event {
    private float yaw;

    public JumpEvent(float yaw) {
        this.yaw = yaw;
    }

    public float getYaw() {
        return yaw;
    }

    public void setYaw(float yaw) {
        this.yaw = yaw;
    }
}
