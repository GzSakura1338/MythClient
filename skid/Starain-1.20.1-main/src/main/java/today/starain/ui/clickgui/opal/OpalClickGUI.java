package today.starain.ui.clickgui.opal;

import com.mojang.blaze3d.platform.InputConstants;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.Component;
import org.lwjgl.glfw.GLFW;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.impl.client.ClickGuiModule;
import today.starain.ui.clickgui.opal.component.CategoryPanel;
import today.starain.ui.clickgui.opal.component.ConfigCategoryPanel;
import today.starain.ui.clickgui.opal.component.OpalComponent;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.context.SkiaContext;

import java.util.ArrayList;
import java.util.List;

public final class OpalClickGUI extends Screen {

    private static final float SEARCH_BAR_HEIGHT = 24f;
    public static boolean displayingBinds;
    private final List<OpalComponent> categoryPanels = new ArrayList<>();
    private final StringBuilder searchQuery = new StringBuilder();
    private boolean closing;
    private boolean searchActive;
    private Animation searchAnimation;

    public OpalClickGUI() {
        super(Component.empty());
        Category[] categories = Category.values();
        for (int i = 0; i < categories.length; i++) {
            if (categories[i] == Category.CONFIG) {
                categoryPanels.add(new ConfigCategoryPanel(i, categories.length));
            } else {
                categoryPanels.add(new CategoryPanel(categories[i], i, categories.length));
            }
        }
    }

    private static boolean isControlDown() {
        long handle = Minecraft.getInstance().getWindow().getWindow();
        return InputConstants.isKeyDown(handle, GLFW.GLFW_KEY_LEFT_CONTROL)
                || InputConstants.isKeyDown(handle, GLFW.GLFW_KEY_RIGHT_CONTROL);
    }

    @Override
    protected void init() {
        closing = false;
        searchActive = false;
        searchQuery.setLength(0);
        searchAnimation = new Animation(Easing.EASE_OUT_SINE, 260);
        categoryPanels.forEach(OpalComponent::init);
    }

    @Override
    public void renderBackground(GuiGraphics guiGraphics) {
    }

    @Override
    public void render(GuiGraphics g, int mouseX, int mouseY, float partialTick) {
        long handle = Minecraft.getInstance().getWindow().getWindow();
        displayingBinds = InputConstants.isKeyDown(handle, GLFW.GLFW_KEY_TAB);

        searchAnimation.run(searchActive ? 1f : 0f);
        float searchProgress = searchAnimation.getValue();
        float searchOffset = searchProgress * SEARCH_BAR_HEIGHT;
        String query = searchActive ? searchQuery.toString() : "";

        SkiaContext.draw(context -> {
            Skia.save();
            Skia.scale((float) Minecraft.getInstance().getWindow().getGuiScale());

            float scaledWidth = Minecraft.getInstance().getWindow().getGuiScaledWidth();

            int count = categoryPanels.size();
            float width = OpalTheme.panelWidth();
            float spacing = OpalTheme.PANEL_SPACING;
            float height = OpalTheme.ROW_HEIGHT;
            float y = OpalTheme.PANEL_TOP + searchOffset;
            float totalWidth = count * width + (count - 1) * spacing;
            float startX = (scaledWidth - totalWidth) / 2f;

            for (int i = 0; i < count; i++) {
                OpalComponent panel = categoryPanels.get(i);
                panel.setSearchQuery(query);
                panel.setDimensions(startX + i * (width + spacing), y, width, height);
                panel.render(mouseX, mouseY, partialTick);
            }

            if (searchProgress > 0.01f) {
                drawSearchBar(startX, totalWidth, searchProgress);
            }

            String hint = searchActive ? "ESC to exit search" : "Ctrl+F to search";
            float hintW = OpalDraw.textWidth(hint, OpalTheme.FONT_BIND, false);
            float scaledH = Minecraft.getInstance().getWindow().getGuiScaledHeight();
            OpalDraw.text(hint, scaledWidth - hintW - 8f, scaledH - 7f, OpalTheme.FONT_BIND, OpalTheme.TEXT_MUTED, false);

            Skia.restore();
        });
    }

    @Override
    public boolean mouseClicked(double mouseX, double mouseY, int button) {
        categoryPanels.forEach(panel -> panel.mouseClicked(mouseX, mouseY, button));
        return true;
    }

    @Override
    public boolean mouseReleased(double mouseX, double mouseY, int button) {
        categoryPanels.forEach(panel -> panel.mouseReleased(mouseX, mouseY, button));
        return true;
    }

    @Override
    public boolean mouseScrolled(double mouseX, double mouseY, double delta) {
        categoryPanels.forEach(panel -> panel.mouseScrolled(mouseX, mouseY, delta));
        return true;
    }

    @Override
    public boolean keyPressed(int key, int scanCode, int modifiers) {
        if (key == GLFW.GLFW_KEY_F && isControlDown()) {
            searchActive = !searchActive;
            if (!searchActive) {
                searchQuery.setLength(0);
            }
            return true;
        }
        if (searchActive) {
            if (key == GLFW.GLFW_KEY_ESCAPE) {
                searchActive = false;
                searchQuery.setLength(0);
                return true;
            }
            if (key == GLFW.GLFW_KEY_BACKSPACE) {
                if (searchQuery.length() > 0) {
                    searchQuery.setLength(searchQuery.length() - 1);
                }
                return true;
            }
            return true;
        }
        if (key == GLFW.GLFW_KEY_ESCAPE) {
            onClose();
            return true;
        }
        categoryPanels.forEach(panel -> panel.keyPressed(key));
        return super.keyPressed(key, scanCode, modifiers);
    }

    @Override
    public boolean charTyped(char chr, int modifiers) {
        if (searchActive) {
            if (chr >= 32 && chr != 127) {
                searchQuery.append(chr);
            }
            return true;
        }
        categoryPanels.forEach(panel -> panel.charTyped(chr));
        return true;
    }

    private void drawSearchBar(float fullX, float fullW, float progress) {
        float w = Math.max(120f, fullW * 0.5f);
        float x = fullX + (fullW - w) / 2f;
        float h = SEARCH_BAR_HEIGHT - 6f;
        float top = OpalTheme.PANEL_TOP - (1f - progress) * 6f;
        float r = OpalTheme.radius();
        OpalDraw.alpha(progress, () -> {
            Skia.drawGuiRoundedBlur(x, top, w, h, r);
            Skia.drawRoundedRect(x, top, w, h, r,
                    OpalDraw.color(OpalDraw.opacity(OpalTheme.ROW_BG, OpalTheme.rowBgOpacity())));

            OpalDraw.centeredIcon("", x + 9.5f, top + h / 2f, OpalTheme.FONT_ROW + 2f, OpalTheme.TEXT_MUTED);

            float textX = x + 18f;
            float textY = top + h / 2f + 3f;
            String q = searchQuery.toString();
            if (q.isEmpty()) {
                OpalDraw.text("Search modules...", textX, textY, OpalTheme.FONT_ROW, OpalTheme.TEXT_MUTED, false);
            } else {
                OpalDraw.text(q, textX, textY, OpalTheme.FONT_ROW, OpalTheme.TEXT, false);
                float caretX = textX + OpalDraw.textWidth(q, OpalTheme.FONT_ROW, false) + 1f;
                float caretAlpha = (float) (0.5 + 0.5 * Math.sin(System.currentTimeMillis() / 220.0));
                int caretArgb = ((int) (caretAlpha * 255f) << 24) | 0xFFFFFF;
                OpalDraw.text("|", caretX, textY, OpalTheme.FONT_ROW, caretArgb, false);
            }

            int accent = OpalTheme.accent(0);
            Skia.drawRect(x + r, top + h - 1.3f, w - r * 2f, 1.3f, OpalDraw.opacity(accent, 0.9f));
        });
    }

    @Override
    public boolean isPauseScreen() {
        return false;
    }

    @Override
    public void onClose() {
        if (closing) {
            return;
        }
        closing = true;
        categoryPanels.forEach(OpalComponent::close);
        Minecraft.getInstance().setScreen(null);
    }

    @Override
    public void removed() {
        super.removed();
        ClickGuiModule.setBindingTarget(null);
        Starain.getInstance().getConfigManager().saveConfig();
    }
}
