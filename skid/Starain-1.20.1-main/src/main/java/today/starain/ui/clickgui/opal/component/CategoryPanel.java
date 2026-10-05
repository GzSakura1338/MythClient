package today.starain.ui.clickgui.opal.component;

import net.minecraft.client.Minecraft;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.ui.clickgui.opal.OpalTheme;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Fonts;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

public final class CategoryPanel extends OpalComponent {

    private final Scroller scroller = new Scroller();
    private final Category category;
    private final int panelIndex;
    private final boolean lastPanel;
    private final List<ModulePanel> modulePanels = new ArrayList<>();
    private String searchQuery = "";
    private Animation openAnimation;
    private boolean closing;

    public CategoryPanel(Category category, int panelIndex, int panelCount) {
        this.category = category;
        this.panelIndex = panelIndex;
        this.lastPanel = panelIndex == panelCount - 1;
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        openAnimation.setDuration(OpalTheme.animOpen(panelIndex));
        openAnimation.run(closing ? 0 : 1);

        if (lastPanel && openAnimation.isFinished() && closing) {
            Minecraft.getInstance().setScreen(null);
            return;
        }

        float[] currentY = {y + height};
        float totalHeight = getTotalHeight();
        float openValue = openAnimation.getValue();
        float scaledHeight = Minecraft.getInstance().getWindow().getGuiScaledHeight();
        float scissorHeight = Math.min(scaledHeight - y, totalHeight * openValue);
        float scrollOffset = scroller.getAnimation().getValue();
        float r = OpalTheme.radius();

        OpalDraw.alpha(openValue, () -> OpalDraw.scissor(x, y, width, scissorHeight, () -> {
            Skia.drawGuiRoundedBlur(x, y + scrollOffset, width, height, r);
            Skia.drawRoundedRectVarying(x, y + scrollOffset, width, height, r, r, 0, 0,
                    OpalDraw.color(OpalDraw.opacity(OpalTheme.HEADER_BG, OpalTheme.headerBgOpacity())));

            OpalDraw.text(category.getName(), x + 5, y + scrollOffset + 13, OpalTheme.FONT_HEADER, OpalTheme.TEXT, true);
            Skia.drawBaselineText(category.getIcon(), x + width - 15.5F, y + scrollOffset + 15, OpalTheme.TEXT, Fonts.getIcon(OpalTheme.ICON_HEADER));

            for (int i = 0; i < modulePanels.size(); i++) {
                ModulePanel panel = modulePanels.get(i);
                float sv = panel.getSearchVisible().getValue();
                float baseHeight = height + (panel.getExpandAnimation().getValue() * panel.getAddedHeight());
                float panelHeight = baseHeight * sv;

                panel.setDimensions(x, currentY[0] + scrollOffset, width, baseHeight);
                panel.setLastModule(i == modulePanels.size() - 1);
                panel.setModuleIndex(i);

                if (sv > 0.001f) {
                    float clipY = currentY[0] + scrollOffset;
                    OpalDraw.alpha(sv, () -> OpalDraw.scissor(x, clipY, width, panelHeight,
                            () -> panel.render(mouseX, mouseY, delta)));
                }

                currentY[0] += panelHeight;
            }
        }));

        scroller.onScroll(getMaxOffset(totalHeight));
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        for (ModulePanel panel : modulePanels) {
            if (matches(panel)) {
                panel.mouseClicked(mouseX, mouseY, button);
            }
        }
    }

    @Override
    public void mouseReleased(double mouseX, double mouseY, int button) {
        for (ModulePanel panel : modulePanels) {
            if (matches(panel)) {
                panel.mouseReleased(mouseX, mouseY, button);
            }
        }
    }

    @Override
    public void keyPressed(int key) {
        modulePanels.forEach(panel -> panel.keyPressed(key));
    }

    @Override
    public void charTyped(int codepoint) {
        modulePanels.forEach(panel -> panel.charTyped(codepoint));
    }

    @Override
    public void mouseScrolled(double mouseX, double mouseY, double verticalAmount) {
        float totalHeight = getTotalHeight();
        if (isHovering(x, y, width, totalHeight, mouseX, mouseY)) {
            scroller.addScroll(verticalAmount, getMaxOffset(totalHeight));
        }
        for (ModulePanel panel : modulePanels) {
            if (matches(panel)) {
                panel.mouseScrolled(mouseX, mouseY, verticalAmount);
            }
        }
    }

    @Override
    public void init() {
        if (modulePanels.isEmpty()) {
            Starain.getInstance().getModuleManager().getModules(category)
                    .forEach(module -> modulePanels.add(new ModulePanel(module)));
            modulePanels.sort(Comparator.comparing(panel -> panel.getModule().getName()));
        }
        this.openAnimation = new Animation(Easing.EASE_OUT_SINE, OpalTheme.animOpen(panelIndex));
        this.closing = false;
        modulePanels.forEach(ModulePanel::init);
    }

    @Override
    public void close() {
        closing = true;
        modulePanels.forEach(ModulePanel::close);
    }

    @Override
    public void setSearchQuery(String query) {
        this.searchQuery = query == null ? "" : query;
    }

    private boolean matches(ModulePanel panel) {
        if (searchQuery.isEmpty()) {
            return true;
        }
        String q = searchQuery.toLowerCase();
        return panel.getModule().getName().toLowerCase().contains(q)
                || panel.getModule().getName().toLowerCase().contains(q);
    }

    private float getTotalHeight() {
        float totalHeight = this.height;
        for (ModulePanel panel : modulePanels) {
            panel.getExpandAnimation().run(panel.isExpanded() ? 1 : 0);
            panel.getSearchVisible().run(matches(panel) ? 1f : 0f);
            float sv = panel.getSearchVisible().getValue();
            totalHeight += (this.height + (panel.getExpandAnimation().getValue() * panel.getAddedHeight())) * sv;
        }
        return totalHeight;
    }

    private float getMaxOffset(float totalHeight) {
        float relativeScreenHeight = Minecraft.getInstance().getWindow().getGuiScaledHeight() - y;
        float scissorHeight = Math.min(relativeScreenHeight, totalHeight * openAnimation.getValue());
        float overflowPadding = scissorHeight == relativeScreenHeight ? y : 0;
        return Math.max(0, totalHeight - scissorHeight + overflowPadding);
    }
}
