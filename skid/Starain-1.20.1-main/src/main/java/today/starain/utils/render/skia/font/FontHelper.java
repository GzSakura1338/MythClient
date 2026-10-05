package today.starain.utils.render.skia.font;

import io.github.humbleui.skija.Data;
import io.github.humbleui.skija.Font;
import io.github.humbleui.skija.FontEdging;
import io.github.humbleui.skija.Typeface;
import today.starain.utils.render.skia.utils.SkiaUtils;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

public class FontHelper {

    private static final Map<String, Typeface> typefaceCache = new HashMap<>();

    private static Typeface getTypeface(String font, FontType type) {
        return typefaceCache.computeIfAbsent(font, k -> loadTypeface(k, type));
    }

    private static Typeface loadTypeface(String font, FontType type) {
        Optional<Data> fontDataOptional = SkiaUtils.convertToData("/assets/starain/fonts/" + font);
        return fontDataOptional.map(Typeface::makeFromData)
                .orElseThrow(() -> new IllegalArgumentException("Font not found: " + font));
    }

    public static Font load(String font, float size, FontType fontType) {
        Typeface typeface = getTypeface(font, fontType);
        Font load = new Font(typeface, size);
        load.setEdging(FontEdging.SUBPIXEL_ANTI_ALIAS);
        load.setSubpixel(true);
        return load;
    }

    public static Font load(String font, float size) {
        return load(font, size, getFontType(font));
    }

    private static FontType getFontType(String font) {
        String fileExtension = font.substring(font.lastIndexOf('.') + 1).toLowerCase();
        return FontType.fromString(fileExtension);
    }

    public static void clearCache() {
        typefaceCache.clear();
    }

    public static void preloadFonts(String... fonts) {
        for (String font : fonts) {
            getTypeface(font, getFontType(font));
        }
    }
}