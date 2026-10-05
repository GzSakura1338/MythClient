package dev.myth.event.impl;

import com.mojang.blaze3d.vertex.PoseStack;
import dev.myth.event.api.Event;
import net.fabricmc.fabric.api.client.rendering.v1.level.LevelRenderContext;

public class RenderWorldEvent implements Event {
    private final float partialTick;
    private final PoseStack poseStack;
    private final LevelRenderContext context;

    public RenderWorldEvent(float partialTick, PoseStack poseStack) {
        this(partialTick, poseStack, null);
    }

    public RenderWorldEvent(float partialTick, PoseStack poseStack, LevelRenderContext context) {
        this.partialTick = partialTick;
        this.poseStack = poseStack;
        this.context = context;
    }

    public float getPartialTick() {
        return partialTick;
    }

    public PoseStack getPoseStack() {
        return poseStack;
    }

    public LevelRenderContext getContext() {
        return context;
    }
}
