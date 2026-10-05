package dev.myth.UI;

import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.input.CharacterEvent;
import net.minecraft.network.chat.Component;
import java.nio.file.Path;
import java.util.List;

/** Vanilla screen owns mouse capture and game-input isolation; ImGui draws at presentation. */
public final class ClickGuiScreen extends Screen {
    private final UiManager manager;
    private final Screen previous;
    ClickGuiScreen(UiManager manager, Screen previous) {
        super(Component.literal("Myth ClickGUI"));
        this.manager = manager; this.previous = previous;
    }
    @Override public void extractBackground(GuiGraphicsExtractor graphics, int mouseX, int mouseY, float delta) {
        if (previous != null && minecraft.level == null)
            previous.extractRenderStateWithTooltipAndSubtitles(graphics, -99, -99, delta);
    }
    @Override public void extractRenderState(GuiGraphicsExtractor graphics, int mouseX, int mouseY, float delta) {}
    @Override public boolean charTyped(CharacterEvent event) { manager.character(event.codepoint()); return true; }
    @Override public boolean mouseScrolled(double x, double y, double sx, double sy) { manager.scroll(sx, sy); return true; }
    @Override public void onFilesDrop(List<Path> paths) { manager.filesDropped(paths); }
    @Override public void onClose() { manager.requestClose(); }
    @Override public boolean isPauseScreen() { return false; }
    @Override public void tick() { manager.tickScreen(this, previous); }
    @Override public void removed() { manager.screenRemoved(); }
}
