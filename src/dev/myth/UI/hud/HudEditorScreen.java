package dev.myth.UI.hud;

import dev.myth.UI.UiManager;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.Component;

/** Minecraft input isolation and return-to-screen lifecycle for the ImGui HUD editor. */
public final class HudEditorScreen extends Screen {
    private final UiManager manager;
    private final Screen previous;
    public HudEditorScreen(UiManager manager, Screen previous) {
        super(Component.literal("Myth HUD Editor")); this.manager = manager; this.previous = previous;
    }
    @Override public void extractBackground(GuiGraphicsExtractor graphics, int mouseX, int mouseY, float delta) {
        if (previous != null && minecraft.level == null) previous.extractRenderStateWithTooltipAndSubtitles(graphics, -99, -99, delta);
    }
    @Override public void extractRenderState(GuiGraphicsExtractor graphics, int mouseX, int mouseY, float delta) {}
    @Override public boolean mouseScrolled(double x, double y, double sx, double sy) { manager.scroll(sx, sy); return true; }
    @Override public void onClose() { manager.requestClose(); }
    @Override public boolean isPauseScreen() { return false; }
    @Override public void tick() { manager.tickScreen(this, previous); }
    @Override public void removed() { manager.screenRemoved(); }
}
