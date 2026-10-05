package dev.myth.event.impl;

import dev.myth.event.api.Event;
import dev.myth.event.api.EventType;

public class GameTickEvent implements Event {
    private final EventType type;

    public GameTickEvent(EventType type) {
        this.type = type;
    }

    public EventType getType() {
        return type;
    }
}
