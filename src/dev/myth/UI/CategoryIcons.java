package dev.myth.UI;

import dev.myth.UI.font.FontManager;
import imgui.ImDrawList;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.util.LinkedHashMap;
import java.util.Map;
import javax.imageio.ImageIO;
import org.lwjgl.system.MemoryUtil;
import static org.lwjgl.opengl.GL33C.*;

/** Bundled Google Material Icons, independent of the selected text font. */
public final class CategoryIcons implements AutoCloseable {
    private static final float SIZE = 10;
    private final Map<String, Integer> textures = new LinkedHashMap<>();

    public void drawTitle(ImDrawList draw, String icon, String title, FontManager fonts, float x, float y, float width, float opacity) {
        int color = Math.clamp(Math.round(255 * opacity), 0, 255) << 24 | 0xffffff;
        float left = x + (width - SIZE - 3 - fonts.width(title)) / 2;
        float textX = left + SIZE + 3, textY = fonts.centeredY(title, y, 14);
        // Header alignment belongs to the frame. Font offsets still apply to
        // controls, while titles stay centered alongside their icons.
        if (fonts.isEnabled()) { textX -= fonts.translate.get(); textY -= fonts.shift.get() + 2; }
        draw.addImage(textures.computeIfAbsent(icon, CategoryIcons::load), left, y + 2, left + SIZE, y + 2 + SIZE,
                0, 0, 1, 1, color);
        fonts.draw(draw, title, textX, textY, 0xffffffff, opacity);
    }

    private static int load(String name) {
        String path = "/assets/myth/icons/" + name + ".png";
        try (var stream = CategoryIcons.class.getResourceAsStream(path)) {
            if (stream == null) throw new IOException("Missing category icon: " + path);
            BufferedImage image = ImageIO.read(stream);
            if (image == null) throw new IOException("Invalid category icon: " + path);
            return upload(image);
        } catch (IOException exception) { throw new IllegalStateException("Could not load category icon " + name, exception); }
    }

    private static int upload(BufferedImage image) {
        // Minecraft can leave a pixel buffer and unpack offsets bound. Preserve
        // them so loading an icon cannot change font or game texture uploads.
        int previous = glGetInteger(GL_TEXTURE_BINDING_2D), buffer = glGetInteger(GL_PIXEL_UNPACK_BUFFER_BINDING);
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
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            return texture;
        } finally {
            MemoryUtil.memFree(pixels);
            glBindTexture(GL_TEXTURE_2D, previous); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer);
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment); glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows); glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        }
    }

    @Override public void close() {
        for (int texture : textures.values()) glDeleteTextures(texture);
        textures.clear();
    }
}
