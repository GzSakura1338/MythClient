package today.starain.event.impl;

import lombok.Getter;
import lombok.Setter;
import today.starain.event.api.Cancellable;
import today.starain.event.api.Event;

@Getter
@Setter
public class ClickEvent implements Event, Cancellable {
    private boolean cancelled;
}
