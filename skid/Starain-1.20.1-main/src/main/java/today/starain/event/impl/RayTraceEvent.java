package today.starain.event.impl;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import net.minecraft.world.entity.Entity;
import today.starain.event.api.Event;

@Getter
@Setter
@AllArgsConstructor
public class RayTraceEvent implements Event {
    public Entity entity;
    public float yaw;
    public float pitch;
}
