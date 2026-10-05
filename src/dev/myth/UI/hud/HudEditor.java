package dev.myth.UI.hud;

import dev.myth.UI.MioDraw;
import dev.myth.UI.CategoryIcons;
import dev.myth.UI.UiOptions;
import dev.myth.UI.font.FontManager;
import imgui.ImDrawList;
import imgui.ImGui;
import imgui.flag.ImGuiMouseCursor;
import net.minecraft.client.input.KeyEvent;
import org.lwjgl.glfw.GLFW;

/** Openmio HUDSearchHelper4 editor shell, with an empty HUD panel by default. */
public final class HudEditor {
    private final HudManager manager;
    private final CategoryIcons icons;
    private final HudSafeArea safeArea;
    private HudElement dragging, hovered;
    private boolean draggingPanel;
    private float dragX, dragY, width, height, panelWidth;

    public HudEditor(HudManager manager, CategoryIcons icons) { this.manager = manager; this.icons = icons; safeArea = new HudSafeArea(manager); }
    public void resetInput() { dragging = hovered = null; draggingPanel = false; safeArea.reset(); }
    public void render(UiOptions options, FontManager fonts, float opacity, boolean interactive) {
        width = ImGui.getIO().getDisplaySizeX(); height = ImGui.getIO().getDisplaySizeY();
        float mouseX = ImGui.getMousePosX(), mouseY = ImGui.getMousePosY();
        ImDrawList draw = ImGui.getBackgroundDrawList();
        int guide = packed(0x66ffffff, opacity);
        MioDraw.rect(draw, width / 2 - .5f, 0, width / 2 + .5f, height, guide);
        MioDraw.rect(draw, 0, height / 2 - .5f, width / 2 - .5f, height / 2 + .5f, guide);
        MioDraw.rect(draw, width / 2 + .5f, height / 2 - .5f, width, height / 2 + .5f, guide);
        safeArea.render(draw, mouseX, mouseY, width, height, opacity, interactive);
        manager.layout(fonts, width, height);
        hovered = null;
        for (HudAnchor anchor : HudAnchor.values()) for (HudElement element : manager.getElements(anchor))
            if (element.contains(mouseX, mouseY)) hovered = element;
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
        for (HudElement element : manager.getElements()) if (element.isEnabled()) {
            MioDraw.rect(draw, element.getX(), element.getY(), element.getX() + Math.max(4, element.getWidth()),
                    element.getY() + Math.max(4, element.getHeight()), packed(0x78000000, opacity));
        }
        // Keep the editor backdrop behind future element content, matching Mio's
        // flat 2D editor layering. The registry is empty until a real HUD element
        // is explicitly registered.
        manager.render(fonts, width, height, opacity, true);
        for (HudElement element : manager.getElements()) if (element.isEnabled()
                && (element == hovered || element == dragging))
            MioDraw.outline(draw, element.getX() - 1, element.getY() - 1,
                    element.getX() + Math.max(4, element.getWidth()), element.getY() + Math.max(4, element.getHeight()),
                    packed(options.themeColor(), opacity));
        panelWidth = options.width();
        float x = manager.getPanelX(), y = manager.getPanelY();
        if (draggingPanel) {
            if (ImGui.isMouseDown(0)) {
                x = Math.clamp(mouseX - dragX, 0, Math.max(0, width - panelWidth)); y = Math.clamp(mouseY - dragY, 0, Math.max(0, height - 14));
                manager.setPanel(x, y, manager.isPanelExpanded());
            } else draggingPanel = false;
        }
        boolean header = mouseX > x && mouseX < x + panelWidth && mouseY > y && mouseY < y + 14;
        dev.myth.Myth.UI_MANAGER.drawBlur(draw, x, y, panelWidth, 14);
        MioDraw.rect(draw, x, y, x + panelWidth, y + 14, packed(options.color(options.bgColor), opacity));
        MioDraw.rect(draw, x, y, x + panelWidth, y + 14, packed(options.themeColor(), opacity));
        icons.drawTitle(draw, "hud", "HUD", fonts, x, y, panelWidth, opacity);
        // This first migration only draws the empty HUD category shell.
        if (options.line.get()) MioDraw.outline(draw, x - 1, y - 1, x + panelWidth, y + 14, packed(options.themeColor(), opacity));
        if (interactive && header && ImGui.isMouseClicked(0)) { draggingPanel = true; dragX = mouseX - x; dragY = mouseY - y; }
        else if (interactive && header && ImGui.isMouseClicked(1)) manager.setPanel(x, y, !manager.isPanelExpanded());
        else if (interactive && !draggingPanel && hovered != null && ImGui.isMouseClicked(0)) {
            dragging = hovered; dragX = mouseX - hovered.getX(); dragY = mouseY - hovered.getY(); manager.setAnchor(dragging, HudAnchor.NONE);
        }
        else if (interactive && !draggingPanel && ImGui.isMouseClicked(0)) safeArea.begin(mouseX, mouseY, width, height);
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
    public boolean key(KeyEvent key) {
        float mouseX = ImGui.getMousePosX(), mouseY = ImGui.getMousePosY();
        if (mouseX >= manager.getPanelX() && mouseX <= manager.getPanelX() + panelWidth
                && mouseY >= manager.getPanelY() && mouseY <= manager.getPanelY() + 14) return false;
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
