package today.starain.event.impl;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import today.starain.event.api.Event;

@Getter
@Setter
@AllArgsConstructor
public class MoveInputEvent implements Event {
    private float forward;
    private float strafe;
    private boolean jump;
    private boolean sneak;
    private double sneakSlowDownMultiplier;

}
