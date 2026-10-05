package today.starain.event.impl;

import lombok.Getter;
import lombok.Setter;
import today.starain.event.api.CancellableEvent;

@Getter
@Setter
public final class ChatEvent extends CancellableEvent {
    private String message;

    public ChatEvent(String message) {
        this.message = message;
    }
}
