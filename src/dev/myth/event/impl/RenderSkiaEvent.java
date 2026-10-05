package dev.myth.event.impl;

import dev.myth.event.api.Event;
import net.minecraft.client.gui.GuiGraphicsExtractor;

public class RenderSkiaEvent implements Event {
    private GuiGraphicsExtractor guiGraphics;
    private float partialTick;

    public RenderSkiaEvent(GuiGraphicsExtractor guiGraphics, float partialTick) {
        this.guiGraphics = guiGraphics;
        this.partialTick = partialTick;
    }

    public GuiGraphicsExtractor getGuiGraphics() {
        return guiGraphics;
    }

    public void setGuiGraphics(GuiGraphicsExtractor guiGraphics) {
        this.guiGraphics = guiGraphics;
    }

    public float getPartialTick() {
        return partialTick;
    }

    public void setPartialTick(float partialTick) {
        this.partialTick = partialTick;
    }
}
