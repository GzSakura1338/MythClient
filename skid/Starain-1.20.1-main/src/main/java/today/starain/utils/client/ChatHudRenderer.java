package today.starain.utils.client;

import net.minecraft.client.GuiMessage;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.ChatComponent;
import net.minecraft.client.gui.screens.ChatScreen;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.player.ChatVisiblity;
import org.joml.Matrix4f;
import org.joml.Vector3f;
import today.starain.interfaces.ChatComponentDuck;
import today.starain.module.impl.client.HUD;
import today.starain.ui.Design;
import today.starain.utils.render.skia.Skia;

import java.util.List;

public final class ChatHudRenderer {

    private static final float PADDING = 3.0f;
    public static final float MARGIN = 5.0f;
    private static final float EXIT_RATE = 20.0f;

    private static float animHeight;
    private static float animWidth;
    private static float animScroll;
    private static float scrollTarget;
    private static float maxScroll;
    private static float messageOffset;
    private static float animInputWidth;
    private static float inputTargetWidth;
    private static int pendingLines;
    private static int hitBase;
    private static int hitRows;
    private static int recentLines;
    private static long lastDisappear;
    private static float lastWidth;
    private static long lastNano;

    private static float panelX;
    private static float panelY;
    private static float panelWidth;
    private static float panelHeight;
    private static boolean panelValid;

    private static int suggestionColor;
    private static float suggestionMinX;
    private static float suggestionMinY;
    private static float suggestionMaxX;
    private static float suggestionMaxY;
    private static boolean suggestionValid;

    private static float inputX;
    private static float inputY;
    private static float inputWidth;
    private static float inputHeight;
    private static boolean inputValid;

    private ChatHudRenderer() {
    }

    public static boolean isActive() {
        return HUD.isChatHudEnabled();
    }

    public static int wrapWidth(ChatComponent chat) {
        return (int) ((HUD.getChatHudWidth() - 10.0f) * chat.getScale());
    }

    public static int scrollBase() {
        return hitBase;
    }

    public static int visibleRows() {
        return hitRows;
    }

    public static int wrapLineWidth() {
        return (int) (HUD.getChatHudWidth() - 10.0f);
    }

    public static void onMessage(int added) {
        pendingLines += Math.max(1, added);
    }

    public static void pushInput(int top, int bottom, int textWidth) {
        inputTargetWidth = Math.min(HUD.getChatHudWidth(), Math.max(90.0f, textWidth + 14.0f));
        if (animInputWidth <= 0.0f) {
            animInputWidth = inputTargetWidth;
        }
        inputX = MARGIN;
        inputY = top - MARGIN;
        inputWidth = animInputWidth;
        inputHeight = bottom - top;
        inputValid = true;
    }

    public static void beginSuggestions(int fillColor) {
        suggestionColor = fillColor;
        suggestionValid = false;
        suggestionMinX = Float.MAX_VALUE;
        suggestionMinY = Float.MAX_VALUE;
        suggestionMaxX = -Float.MAX_VALUE;
        suggestionMaxY = -Float.MAX_VALUE;
    }

    public static boolean captureSuggestion(int x1, int y1, int x2, int y2, int color) {
        if (!isActive() || color != suggestionColor) {
            return false;
        }
        suggestionValid = true;
        suggestionMinX = Math.min(suggestionMinX, Math.min(x1, x2) + MARGIN);
        suggestionMaxX = Math.max(suggestionMaxX, Math.max(x1, x2) + MARGIN);
        suggestionMinY = Math.min(suggestionMinY, Math.min(y1, y2) - MARGIN);
        suggestionMaxY = Math.max(suggestionMaxY, Math.max(y1, y2) - MARGIN);
        return true;
    }

    public static boolean scroll(double amount) {
        if (!isActive()) {
            return false;
        }
        scrollTarget = Mth.clamp(scrollTarget + (float) amount, 0.0f, maxScroll);
        return true;
    }

    public static void renderPanel() {
        if (!isActive() || !HUD.isChatBackgroundEnabled()) {
            return;
        }
        if (panelValid && panelWidth >= 1.0f && panelHeight >= 1.0f) {
            drawSurface(panelX, panelY, panelWidth, panelHeight);
        }
        if (inputValid && inputWidth >= 1.0f && inputHeight >= 1.0f) {
            drawSurface(inputX, inputY, inputWidth, inputHeight);
        }
        if (suggestionValid) {
            float sx = suggestionMinX - 2.0f;
            float sy = suggestionMinY - 2.0f;
            float sw = suggestionMaxX - suggestionMinX + 4.0f;
            float sh = suggestionMaxY - suggestionMinY + 4.0f;
            if (sw >= 1.0f && sh >= 1.0f) {
                drawSurface(sx, sy, sw, sh);
            }
        }
    }

    private static void drawSurface(float x, float y, float width, float height) {
        float radius = Math.min(6.0f, height / 2.0f);
        Skia.drawShadow(x, y, width, height, radius);
        Skia.drawBlur(x, y, width, height, radius);
        Skia.drawRoundedRect(x, y, width, height, radius, Design.BACKGROUND_COLOR);
    }

    public static void render(ChatComponent chat, GuiGraphics graphics, int tickCount) {
        Minecraft mc = Minecraft.getInstance();
        panelValid = false;
        boolean focused = mc.screen instanceof ChatScreen;
        if (!focused) {
            inputValid = false;
            suggestionValid = false;
            scrollTarget = 0.0f;
            animInputWidth = 0.0f;
        }
        if (mc.options.chatVisibility().get() == ChatVisiblity.HIDDEN) {
            return;
        }

        float delta = advance();
        float factor = 1.0f - (float) Math.exp(-delta * 14.0d);
        syncWidth(chat);

        List<GuiMessage.Line> lines = ((ChatComponentDuck) chat).starain$lines();
        float scale = (float) chat.getScale();
        double spacing = mc.options.chatLineSpacing().get();
        int lineHeight = (int) (9.0d * (spacing + 1.0d));
        int textOffset = (int) Math.round(-8.0d * (spacing + 1.0d) + 4.0d * spacing);
        float textOpacity = (float) (double) mc.options.chatOpacity().get() * 0.9f + 0.1f;

        animInputWidth += (inputTargetWidth - animInputWidth) * factor;

        float openLocal = HUD.getChatHudOpenHeight() / scale;
        float closedLocal = HUD.getChatHudClosedHeight() / scale;
        int closedRows = Math.max(0, Mth.floor(closedLocal / lineHeight));

        if (pendingLines > 0) {
            if (scrollTarget <= 0.0f) {
                messageOffset = Math.min(messageOffset + pendingLines * (float) lineHeight, lineHeight * 3.0f);
            }
            recentLines = Math.min(recentLines + pendingLines, closedRows);
            pendingLines = 0;
        }
        recentLines = Math.min(recentLines, lines.size());
        messageOffset -= messageOffset * factor;

        long now = System.currentTimeMillis();
        if (recentLines <= 0) {
            lastDisappear = now;
        } else if (now - lastDisappear >= (long) HUD.getChatHudDuration()) {
            lastDisappear = now;
            recentLines--;
        }

        float maxHeight = focused ? openLocal : closedLocal;
        int visible = focused
                ? Math.min(lines.size(), Mth.floor(openLocal / lineHeight))
                : Math.min(lines.size(), recentLines);
        float targetHeight = Math.min(maxHeight, visible * (float) lineHeight);
        boolean closing = targetHeight <= 0.01f;
        float heightFactor = closing ? 1.0f - (float) Math.exp(-delta * EXIT_RATE) : factor;
        animHeight += (targetHeight - animHeight) * heightFactor;

        int visibleRows = Mth.floor(maxHeight / lineHeight);
        maxScroll = Math.max(0.0f, lines.size() - visibleRows);
        scrollTarget = Mth.clamp(scrollTarget, 0.0f, maxScroll);
        animScroll += (scrollTarget - animScroll) * factor;

        if (closing && animHeight < 0.5f) {
            animHeight = 0.0f;
            animWidth = 0.0f;
            animScroll = 0.0f;
        }
        if (animHeight < 0.5f) {
            return;
        }

        int base = Mth.floor(animScroll);
        float fraction = animScroll - base;
        hitBase = base;
        hitRows = visible;
        int rows = Mth.ceil(animHeight / lineHeight) + 2;

        float limit = HUD.getChatHudWidth() / scale;
        float longest = 0.0f;
        for (int row = 0; row < rows; row++) {
            int index = base + row;
            if (index < 0 || index >= lines.size()) {
                continue;
            }
            GuiMessage.Line line = lines.get(index);
            if (line == null || row >= visible) {
                continue;
            }
            longest = Math.max(longest, mc.font.width(line.content()));
        }
        float targetWidth = closing ? 0.0f : Math.min(limit, longest + PADDING * 2.0f + 4.0f);
        animWidth += (targetWidth - animWidth) * (closing ? heightFactor : factor);

        graphics.pose().pushPose();
        graphics.pose().scale(scale, scale, 1.0f);
        graphics.pose().translate(4.0f + MARGIN / scale, -MARGIN / scale, 0.0f);

        int bottom = Mth.floor((graphics.guiHeight() - 40) / scale);
        float localWidth = animWidth;
        float localX = -4.0f;
        float localY = bottom - animHeight - PADDING;
        float localHeight = animHeight + PADDING * 2.0f;

        Matrix4f matrix = graphics.pose().last().pose();
        Vector3f topLeft = matrix.transformPosition(new Vector3f(localX, localY, 0.0f));
        Vector3f bottomRight = matrix.transformPosition(new Vector3f(localX + localWidth, localY + localHeight, 0.0f));
        panelX = topLeft.x;
        panelY = topLeft.y;
        panelWidth = bottomRight.x - topLeft.x;
        panelHeight = bottomRight.y - topLeft.y;
        panelValid = true;

        Vector3f contentTop = matrix.transformPosition(new Vector3f(localX, bottom - animHeight, 0.0f));
        Vector3f contentBottom = matrix.transformPosition(new Vector3f(localX + localWidth, (float) bottom, 0.0f));
        graphics.enableScissor(Math.round(contentTop.x), Math.round(contentTop.y),
                Math.round(contentBottom.x), Math.round(contentBottom.y));

        graphics.pose().pushPose();
        graphics.pose().translate(0.0f, fraction * lineHeight + messageOffset, 0.0f);
        for (int row = 0; row < rows; row++) {
            int index = base + row;
            if (index < 0 || index >= lines.size()) {
                continue;
            }
            GuiMessage.Line line = lines.get(index);
            if (line == null) {
                continue;
            }
            int alpha = (int) (255.0f * textOpacity);
            if (alpha <= 3) {
                continue;
            }
            graphics.drawString(mc.font, ChatFlow.recolor(line.content()), 0,
                    bottom - row * lineHeight + textOffset, 0xFFFFFF | (alpha << 24));
        }
        graphics.pose().popPose();

        graphics.disableScissor();
        graphics.pose().popPose();
    }

    private static void syncWidth(ChatComponent chat) {
        float current = HUD.getChatHudWidth();
        if (current != lastWidth) {
            lastWidth = current;
            chat.rescaleChat();
        }
    }

    private static float advance() {
        long now = System.nanoTime();
        float delta = lastNano == 0L ? 0.0f : Math.min(0.05f, (now - lastNano) / 1.0E9f);
        lastNano = now;
        return delta;
    }
}
