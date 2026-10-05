package dev.myth.UI.font;

import imgui.ImDrawList;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import javax.imageio.ImageIO;
import net.minecraft.client.Minecraft;
import net.minecraft.resources.Identifier;
import org.lwjgl.system.MemoryUtil;
import static org.lwjgl.opengl.GL33C.*;

/** Minecraft bitmap fallback when Mio's Fonts toggle is off. */
final class FontAtlas implements AutoCloseable {
    private record Glyph(float u, float v, float u2, float v2, float width, float height, float advance, float textTop, float textBottom) {}
    private final Map<Character, Glyph> glyphs = new HashMap<>();
    private int texture;
    private float height;
    private float textTop;
    private float textBottom;

    static FontAtlas minecraft() {
        FontAtlas atlas = new FontAtlas();
        try (var stream = Minecraft.getInstance().getResourceManager()
                .open(Identifier.withDefaultNamespace("textures/font/ascii.png"))) {
            BufferedImage image = ImageIO.read(stream);
            if (image == null) throw new IOException("Invalid Minecraft ASCII atlas");
            int cellX = image.getWidth() / 16, cellY = image.getHeight() / 16;
            // Isolate each cell so animated quads cannot sample adjacent glyphs.
            int paddedX = cellX + 2, paddedY = cellY + 2;
            BufferedImage padded = new BufferedImage(paddedX * 16, paddedY * 16, BufferedImage.TYPE_INT_ARGB);
            int inkTop = cellY, inkBottom = 0;
            for (int code = 32; code < 127; code++) {
                int x = (code % 16) * cellX, y = (code / 16) * cellY;
                int targetX = (code % 16) * paddedX + 1, targetY = (code / 16) * paddedY + 1;
                int lastColumn = -1;
                int glyphTop = cellY, glyphBottom = 0;
                for (int col = 0; col < cellX; col++) for (int row = 0; row < cellY; row++) {
                    int pixel = image.getRGB(x + col, y + row);
                    padded.setRGB(targetX + col, targetY + row, pixel);
                    if ((pixel >>> 24) == 0) continue;
                    lastColumn = Math.max(lastColumn, col);
                    glyphTop = Math.min(glyphTop, row); glyphBottom = Math.max(glyphBottom, row + 1);
                    if (Character.isLetterOrDigit(code)) {
                        inkTop = Math.min(inkTop, row); inkBottom = Math.max(inkBottom, row + 1);
                    }
                }
                float advance = code == 32 ? 4 : Math.round((lastColumn + 1) * 8f / cellY) + 1;
                atlas.glyphs.put((char) code, new Glyph(targetX / (float) padded.getWidth(), targetY / (float) padded.getHeight(),
                        (targetX + cellX) / (float) padded.getWidth(), (targetY + cellY) / (float) padded.getHeight(), 8f * cellX / cellY, 8, advance,
                        glyphTop * 8f / cellY, glyphBottom * 8f / cellY));
            }
            atlas.height = 9;
            atlas.textTop = inkTop * 8f / cellY; atlas.textBottom = inkBottom * 8f / cellY;
            atlas.upload(padded, false);
            return atlas;
        } catch (IOException exception) {
            throw new IllegalStateException("Could not load Minecraft font", exception);
        }
    }

    private void upload(BufferedImage image, boolean linear) {
        texture = uploadImage(image, linear);
    }

    static int uploadImage(BufferedImage image, boolean linear) {
        int previousTexture = glGetInteger(GL_TEXTURE_BINDING_2D);
        int unpackBuffer = glGetInteger(GL_PIXEL_UNPACK_BUFFER_BINDING);
        int alignment = glGetInteger(GL_UNPACK_ALIGNMENT), rowLength = glGetInteger(GL_UNPACK_ROW_LENGTH);
        int skipRows = glGetInteger(GL_UNPACK_SKIP_ROWS), skipPixels = glGetInteger(GL_UNPACK_SKIP_PIXELS);
        var pixels = MemoryUtil.memAlloc(image.getWidth() * image.getHeight() * 4);
        try {
            for (int y = 0; y < image.getHeight(); y++) for (int x = 0; x < image.getWidth(); x++) {
                int argb = image.getRGB(x, y);
                pixels.put((byte) (argb >> 16)).put((byte) (argb >> 8)).put((byte) argb).put((byte) (argb >>> 24));
            }
            pixels.flip(); int texture = glGenTextures();
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); glBindTexture(GL_TEXTURE_2D, texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, 0); glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.getWidth(), image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            return texture;
        } finally {
            MemoryUtil.memFree(pixels);
            glBindTexture(GL_TEXTURE_2D, previousTexture); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment); glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows); glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        }
    }

    float height() { return height; }
    float textTop(String text) { return textEdge(text, true); }
    float textBottom(String text) { return textEdge(text, false); }
    private float textEdge(String text, boolean top) {
        float edge = top ? Float.POSITIVE_INFINITY : Float.NEGATIVE_INFINITY;
        for (char c : text.toCharArray()) {
            Glyph glyph = glyphs.getOrDefault(c, glyphs.get('?'));
            if (glyph.textBottom() == 0) continue;
            edge = top ? Math.min(edge, glyph.textTop()) : Math.max(edge, glyph.textBottom());
        }
        return Float.isFinite(edge) ? edge : top ? textTop : textBottom;
    }
    float width(String text) {
        float width = 0;
        for (char c : text.toCharArray()) { Glyph glyph = glyphs.getOrDefault(c, glyphs.get('?')); width += glyph.advance(); }
        return width;
    }

    void draw(ImDrawList draw, String text, float x, float y, int argb, float opacity, float scale) {
        int color = (Math.clamp(Math.round((argb >>> 24) * opacity), 0, 255) << 24)
                | ((argb & 0xff) << 16) | (argb & 0xff00) | ((argb >> 16) & 0xff);
        for (char c : text.toCharArray()) {
            Glyph glyph = glyphs.getOrDefault(c, glyphs.get('?'));
            if (c != ' ') draw.addImage(texture, x, y, x + glyph.width() * scale, y + glyph.height() * scale,
                    glyph.u(), glyph.v(), glyph.u2(), glyph.v2(), color);
            x += glyph.advance() * scale;
        }
    }

    @Override public void close() { if (texture != 0) glDeleteTextures(texture); texture = 0; }
}
