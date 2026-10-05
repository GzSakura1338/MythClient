package today.starain.module.impl.client;

import io.github.humbleui.skija.Font;
import today.starain.Starain;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.ui.Design;
import today.starain.ui.Theme;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Fonts;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.NumberValue;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

public final class ArrayListModule extends ClientModule {

    private final NumberValue x = new NumberValue("X", 0, 1000, 10, 1, this);
    private final NumberValue y = new NumberValue("y", 0, 1000, 30, 1, this);
    private final BooleanValue syncColor = new BooleanValue("Sync Color", false, this);
    private final BooleanValue background = new BooleanValue("Background", true, this);
    private final BooleanValue glow = new BooleanValue("Glow", false, this);
    private final BooleanValue sidebar = new BooleanValue("Sidebar", false, this);
    private final BooleanValue hideRenderModule = new BooleanValue("Hide Render Module", true, this);

    public ArrayListModule() {
        setName("ArrayList");
        setCategory(Category.CLIENT);
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {
        List<ClientModule> modules = new ArrayList<>(Starain.getInstance().getModuleManager().getModules());
        Font font = Fonts.getNormal(7.5f);
        modules.sort(Comparator.comparing((m) -> Skia.getTextWidth(((ClientModule) m).getDisplayName(), font)).reversed());
        modules.removeIf((m) -> (m.getCategory() == Category.RENDER || m.getCategory() == Category.CLIENT) && hideRenderModule.get());
        int guiScaledWidth = mc.getWindow().getGuiScaledWidth();
        // 距离屏幕顶部10像素
        float startY = y.get().floatValue();
        for (ClientModule module : modules) {
            module.progress.update(module.isEnabled());
            if (module.progress.value > 0) {
                float textWidth = Skia.getTextWidth(module.getDisplayName(), font);
                // 距离屏幕右边缘10像素 文字间距上下2.5左右3.5
                float width = 3.5F + textWidth + 3.5F;
                float height = 2.5f + 7.5f + 2.5f;
                float x = guiScaledWidth - this.x.get().floatValue() - width * ((module.progress.value - 50f) / 50f);
                if (glow.get()) {
                    today.starain.module.impl.render.Glow.queueHud(x, startY - 0.5F, width, height + 1.0F, 0, module.progress.value / 100.0f);
                }
                if (background.get()) {
                    Skia.drawShadow(x, startY, width, height, 0);
                    Skia.drawBlur(x, startY, width, height, 0);
                    Skia.drawRoundedRect(x, startY, width, height, 0, Design.BACKGROUND_COLOR);
                }
                java.awt.Color textColor = syncColor.get() ? Theme.accent(-startY * 0.1d) : Design.TEXT_COLOR;
                Skia.drawText(module.getDisplayName(), x + 2.5F, startY + 2.5F, textColor, font);
                if (sidebar.get()) {
                    Skia.drawRoundedRect(x + width - 2.5F, startY + 1.5F, 2.0F, height - 3.0F, 1.0F, textColor);
                }
                startY += height * (module.progress.value / 100f);
            }
        }
    }
}
