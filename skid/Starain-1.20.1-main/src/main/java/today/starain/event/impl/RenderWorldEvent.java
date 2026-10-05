package today.starain.event.impl;

import com.mojang.blaze3d.vertex.PoseStack;
import lombok.Getter;
import today.starain.event.api.Event;

@Getter
public class RenderWorldEvent implements Event {
    private final float partialTick;
    private final PoseStack poseStack;

    public RenderWorldEvent(float partialTick, PoseStack poseStack) {
        this.partialTick = partialTick;
        this.poseStack = poseStack;
    }
}
