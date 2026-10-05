package today.starain.event.impl;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import today.starain.event.api.Event;

@Getter
@Setter
@AllArgsConstructor
public class JumpEvent implements Event {
    private float yaw;
}
