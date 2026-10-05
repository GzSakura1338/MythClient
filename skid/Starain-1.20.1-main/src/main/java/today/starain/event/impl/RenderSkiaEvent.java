package today.starain.event.impl;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import net.minecraft.client.gui.GuiGraphics;
import today.starain.event.api.Event;

@Getter
@Setter
@AllArgsConstructor
public class RenderSkiaEvent implements Event {
    private GuiGraphics guiGraphics;
    private float partialTick;
}
