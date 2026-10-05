package dev.myth.UI.hud;

import dev.myth.UI.MioDraw;
import dev.myth.UI.CategoryIcons;
import dev.myth.UI.ClickGui;
import dev.myth.UI.UiOptions;
import dev.myth.UI.font.FontManager;
import imgui.ImDrawList;
import imgui.ImGui;
import imgui.flag.ImGuiMouseCursor;
import net.minecraft.client.input.KeyEvent;
import org.lwjgl.glfw.GLFW;

/** Openmio HUDSearchHelper4 editor with shared Mio module controls. */
public final class HudEditor {
    private final HudManager manager;
    private final CategoryIcons icons;
    private final ClickGui controls;
    private final HudSafeArea safeArea;
    private HudElement dragging, hovered;
    private HudElement resizing;
    private boolean draggingPanel, overPanel;
    private float dragX, dragY, width, height, panelWidth, panelHeight, mouseX, mouseY;

    public HudEditor(HudManager manager, CategoryIcons icons, ClickGui controls) {
        this.manager = manager; this.icons = icons; this.controls = controls; safeArea = new HudSafeArea(manager);
    }
    public void resetInput() { dragging = hovered = resizing = null; draggingPanel = false; safeArea.reset(); }
    /** Guides, element backdrops and the elements themselves: everything the blur may frost. */
    public void renderBelow(UiOptions options, FontManager fonts, float opacity, boolean interactive) {
        width = ImGui.getIO().getDisplaySizeX(); height = ImGui.getIO().getDisplaySizeY();
        mouseX = ImGui.getMousePosX(); mouseY = ImGui.getMousePosY();
        panelWidth = options.width(); panelHeight = controls.prepareHudPanel();
        overPanel = mouseX >= manager.getPanelX() && mouseX <= manager.getPanelX() + panelWidth
                && mouseY >= manager.getPanelY() && mouseY <= manager.getPanelY() + panelHeight;
        ImDrawList draw = ImGui.getBackgroundDrawList();
        int guide = packed(0x66ffffff, opacity);
        MioDraw.rect(draw, width / 2 - .5f, 0, width / 2 + .5f, height, guide);
        MioDraw.rect(draw, 0, height / 2 - .5f, width / 2 - .5f, height / 2 + .5f, guide);
        MioDraw.rect(draw, width / 2 + .5f, height / 2 - .5f, width, height / 2 + .5f, guide);
        safeArea.render(draw, mouseX, mouseY, width, height, opacity, interactive);
        manager.layout(fonts, width, height);
        if (resizing != null) {
            if (ImGui.isMouseDown(0)) {
                var setting = sizeSetting(resizing);
                float baseWidth = resizing.getWidth() / setting.get(), baseHeight = resizing.getHeight() / setting.get();
                setting.set(Math.min((mouseX - resizing.getX()) / baseWidth, (mouseY - resizing.getY()) / baseHeight));
            } else resizing = null;
        }
        hovered = null;
        for (HudAnchor anchor : HudAnchor.values()) for (HudElement element : manager.getElements(anchor))
            if (!overPanel && element.contains(mouseX, mouseY)) hovered = element;
        if (dragging != null) {
            if (ImGui.isMouseDown(0)) {
                float x = Math.clamp(mouseX - dragX, 0, Math.max(0, width - dragging.getWidth()));
                float y = Math.clamp(mouseY - dragY, 0, Math.max(0, height - dragging.getHeight()));
                if (Math.abs(x + dragging.getWidth() / 2 - width / 2) <= 5) {
                    x = (width - dragging.getWidth()) / 2;
                    MioDraw.rect(draw, x - .5f, 0, x, height, packed(0xffffff00, opacity));
                    MioDraw.rect(draw, x + dragging.getWidth(), 0, x + dragging.getWidth() + .5f, height, packed(0xffffff00, opacity));
                }
                if (Math.abs(y + dragging.getHeight() / 2 - height / 2) <= 5) {
                    y = (height - dragging.getHeight()) / 2;
                    MioDraw.rect(draw, 0, y - .5f, width, y, packed(0xffffff00, opacity));
                    MioDraw.rect(draw, 0, y + dragging.getHeight(), width, y + dragging.getHeight() + .5f, packed(0xffffff00, opacity));
                }
                dragging.setPosition(x, y, width, height);
            } else { dock(dragging); dragging = null; }
        }
        for (HudElement element : manager.getElements()) if (element.isEnabled() && element.isEditable()) {
            MioDraw.rect(draw, element.getX(), element.getY(), element.getX() + Math.max(4, element.getWidth()),
                    element.getY() + Math.max(4, element.getHeight()), packed(0x78000000, opacity));
        }
        // Keep the editor backdrop behind element content, matching Mio's flat layering.
        manager.render(fonts, width, height, opacity, true, false);
    }

    /** Blurred elements, selection chrome and the category panel, drawn over the cached blur. */
    public void renderAbove(UiOptions options, FontManager fonts, float opacity, boolean interactive) {
        ImDrawList draw = ImGui.getBackgroundDrawList();
        manager.render(fonts, width, height, opacity, true, true);
        for (HudElement element : manager.getElements()) if (element.isEnabled()
                && (element == hovered || element == dragging))
            MioDraw.outline(draw, element.getX() - 1, element.getY() - 1,
                    element.getX() + Math.max(4, element.getWidth()), element.getY() + Math.max(4, element.getHeight()),
                    packed(options.themeColor(), opacity));
        boolean overResize = false;
        for (HudElement element : manager.getElements()) if (element.isEnabled() && sizeSetting(element) != null) {
            float right = element.getX() + element.getWidth(), bottom = element.getY() + element.getHeight();
            boolean handle = !overPanel && mouseX >= right - 5 && mouseX <= right && mouseY >= bottom - 5 && mouseY <= bottom;
            overResize |= handle;
            int texture = manager.assets.texture("/assets/myth/hud/triangle.png");
            draw.addImage(texture, right - 5, bottom - 5, right, bottom, 0, 0, 1, 1,
                    packed(options.themeColor(), opacity * (resizing == element ? .9f : handle ? .4f : .2f)));
            if (interactive && handle && dev.myth.Myth.UI_MANAGER.isMouseClicked(0)) { resizing = element; dragging = null; }
            if (interactive && handle && dev.myth.Myth.UI_MANAGER.isMouseClicked(1)) sizeSetting(element).set(1f);
        }
        panelWidth = options.width();
        float x = manager.getPanelX(), y = manager.getPanelY();
        if (draggingPanel) {
            if (ImGui.isMouseDown(0)) {
                x = Math.clamp(mouseX - dragX, 0, Math.max(0, width - panelWidth)); y = Math.clamp(mouseY - dragY, 0, Math.max(0, height - panelHeight));
                manager.setPanel(x, y, manager.isPanelExpanded());
            } else draggingPanel = false;
        }
        boolean header = mouseX > x && mouseX < x + panelWidth && mouseY > y && mouseY < y + 14;
        float panelBottom = y + panelHeight + (panelHeight > 14 ? .5f : 0);
        dev.myth.Myth.UI_MANAGER.drawPanelBlur(draw, x, y, panelWidth, panelBottom - y);
        MioDraw.rect(draw, x, y, x + panelWidth, y + 14, packed(options.color(options.bgColor), opacity));
        MioDraw.rect(draw, x, y, x + panelWidth, y + 14, packed(options.themeColor(), opacity));
        icons.drawTitle(draw, "hud", "HUD", fonts, x, y, panelWidth, opacity);
        controls.renderHudRows(x, y, panelWidth, panelHeight, opacity, interactive && !draggingPanel && resizing == null);
        if (options.line.get()) MioDraw.panelOutline(draw, x - 1, y - 1, x + panelWidth, panelBottom,
                packed(options.color(options.bgColor), opacity), packed(options.color(options.lineColor), opacity));
        if (interactive && header && dev.myth.Myth.UI_MANAGER.isMouseClicked(0)) { draggingPanel = true; dragX = mouseX - x; dragY = mouseY - y; }
        else if (interactive && header && dev.myth.Myth.UI_MANAGER.isMouseClicked(1)) manager.setPanel(x, y, !manager.isPanelExpanded());
        else if (interactive && !draggingPanel && !overResize && resizing == null && hovered != null && dev.myth.Myth.UI_MANAGER.isMouseClicked(0)) {
            dragging = hovered; dragX = mouseX - hovered.getX(); dragY = mouseY - hovered.getY(); manager.setAnchor(dragging, HudAnchor.NONE);
        }
        else if (interactive && !draggingPanel && !overPanel && !overResize && resizing == null && dev.myth.Myth.UI_MANAGER.isMouseClicked(0)) safeArea.begin(mouseX, mouseY, width, height);
        ImGui.setMouseCursor(interactive && (header || hovered != null) ? ImGuiMouseCursor.Hand : ImGuiMouseCursor.Arrow);
    }
    private void dock(HudElement element) {
        for (HudAnchor anchor : HudAnchor.values()) if (anchor != HudAnchor.NONE) for (HudElement other : manager.getElements(anchor)) {
            if (other.isEnabled() && element.getX() <= other.getX() + other.getWidth()
                    && element.getX() + element.getWidth() >= other.getX()
                    && element.getY() <= other.getY() + other.getHeight()
                    && element.getY() + element.getHeight() >= other.getY()) { manager.setAnchor(element, anchor); return; }
        }
        boolean top = element.getY() <= 15, bottom = element.getY() + element.getHeight() >= height - 15;
        boolean left = element.getX() <= 15, right = element.getX() + element.getWidth() >= width - 15;
        if (top && left) manager.setAnchor(element, HudAnchor.TOP_LEFT);
        else if (top && right) manager.setAnchor(element, HudAnchor.TOP_RIGHT);
        else if (bottom && left) manager.setAnchor(element, HudAnchor.BOTTOM_LEFT);
        else if (bottom && right) manager.setAnchor(element, HudAnchor.BOTTOM_RIGHT);
        else if (top && Math.abs(element.getX() + element.getWidth() / 2 - width / 2) <= 15) manager.setAnchor(element, HudAnchor.TOP_CENTER);
    }
    @SuppressWarnings("unchecked")
    private static dev.myth.UI.setting.Setting<Float> sizeSetting(HudElement element) {
        return element.getSettings().stream().filter(setting -> setting.getId().equals("size"))
                .map(setting -> (dev.myth.UI.setting.Setting<Float>) setting).findFirst().orElse(null);
    }
    public boolean key(KeyEvent key) {
        float mouseX = ImGui.getMousePosX(), mouseY = ImGui.getMousePosY();
        if (mouseX >= manager.getPanelX() && mouseX <= manager.getPanelX() + panelWidth
                && mouseY >= manager.getPanelY() && mouseY <= manager.getPanelY() + panelHeight) return false;
        if (key.key() == GLFW.GLFW_KEY_I) {
            manager.swap(HudAnchor.TOP_LEFT, HudAnchor.TOP_RIGHT); manager.swap(HudAnchor.BOTTOM_LEFT, HudAnchor.BOTTOM_RIGHT); return true;
        }
        if (hovered == null) return false;
        if (key.key() == GLFW.GLFW_KEY_DELETE || key.key() == GLFW.GLFW_KEY_BACKSPACE) { hovered.setEnabled(false); return true; }
        int x = key.key() == GLFW.GLFW_KEY_RIGHT ? 1 : key.key() == GLFW.GLFW_KEY_LEFT ? -1 : 0;
        int y = key.key() == GLFW.GLFW_KEY_DOWN ? 1 : key.key() == GLFW.GLFW_KEY_UP ? -1 : 0;
        if (x == 0 && y == 0) return false;
        // A free position must survive the next anchored layout pass.
        manager.setAnchor(hovered, HudAnchor.NONE);
        hovered.setPosition(hovered.getX() + x, hovered.getY() + y, width, height); return true;
    }
    public void scroll(double amount) { manager.setPanel(manager.getPanelX(), manager.getPanelY() + (float) amount * 13, manager.isPanelExpanded()); }
    private static int packed(int argb, float opacity) {
        return Math.clamp(Math.round((argb >>> 24) * opacity), 0, 255) << 24 | (argb & 255) << 16 | argb & 0xff00 | argb >> 16 & 255;
    }
}
