package dev.myth.UI.hud;

import dev.myth.UI.MioDraw;
import imgui.ImDrawList;
import imgui.ImGui;

/** Mio ZoomHelper_2: drag either screen edge to adjust SafeX/SafeY. */
final class HudSafeArea {
    private final HudManager manager;
    private boolean draggingX, draggingY, fromRight, fromBottom;
    private float alphaX, alphaY;

    HudSafeArea(HudManager manager) { this.manager = manager; }

    void reset() { draggingX = draggingY = false; alphaX = alphaY = 0; }

    void begin(float x, float y, float width, float height) {
        draggingX = nearX(x, width);
        draggingY = !draggingX && nearY(y, height);
        fromRight = x > width / 2; fromBottom = y > height / 2;
    }

    void render(ImDrawList draw, float x, float y, float width, float height, float opacity, boolean interactive) {
        if (!interactive || !ImGui.isMouseDown(0)) draggingX = draggingY = false;
        if (draggingX) manager.safeX.setNumber(fromRight ? width - x : x);
        if (draggingY) manager.safeY.setNumber(fromBottom ? height - y : y);
        float step = Math.clamp(ImGui.getIO().getDeltaTime() / .25f, 0, 1);
        alphaX = approach(alphaX, interactive && (nearX(x, width) || draggingX) ? 127 : 0, step * 127);
        alphaY = approach(alphaY, interactive && alphaX == 0 && (nearY(y, height) || draggingY) ? 127 : 0, step * 127);
        if (alphaX > 0) {
            int color = Math.round(alphaX * opacity) << 24;
            MioDraw.rect(draw, 0, 0, manager.safeX.get(), height, color);
            MioDraw.rect(draw, width - manager.safeX.get(), 0, width, height, color);
        } else if (alphaY > 0) {
            int color = Math.round(alphaY * opacity) << 24;
            MioDraw.rect(draw, 0, 0, width, manager.safeY.get(), color);
            MioDraw.rect(draw, 0, height - manager.safeY.get(), width, height, color);
        }
    }

    private boolean nearX(float x, float width) { return x >= 0 && x <= width && (x < manager.safeX.get() + 1 || x > width - manager.safeX.get() - 1); }
    private boolean nearY(float y, float height) { return y >= 0 && y <= height && (y < manager.safeY.get() + 1 || y > height - manager.safeY.get() - 1); }
    private static float approach(float value, float target, float step) { return value < target ? Math.min(target, value + step) : Math.max(target, value - step); }
}
