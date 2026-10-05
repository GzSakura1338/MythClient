package me.mioclient.mixin.ducks;

import java.util.List;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.tooltip.TooltipComponent;
import net.minecraft.client.gui.tooltip.TooltipPositioner;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

/* compiled from: 0.java */
@Mixin({DrawContext.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckDrawContext.class */
public interface DuckDrawContext {
    @Invoker("drawTooltip")
    void drawTooltipsHook(TextRenderer textRenderer, List<TooltipComponent> list, int i, int i2, TooltipPositioner tooltipPositioner);
}
