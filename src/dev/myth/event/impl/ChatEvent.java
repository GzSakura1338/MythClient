package dev.myth.event.impl;

import dev.myth.event.api.CancellableEvent;

public final class ChatEvent extends CancellableEvent {
    private String message;

    public ChatEvent(String message) {
        this.message = message;
    }

    public String getMessage() {
        return message;
    }

    public void setMessage(String message) {
        this.message = message;
    }
}
