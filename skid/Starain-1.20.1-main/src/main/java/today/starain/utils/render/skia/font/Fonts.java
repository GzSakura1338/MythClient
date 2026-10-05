package today.starain.utils.render.skia.font;

import io.github.humbleui.skija.Font;

public class Fonts {

    public static Font getNormal(float size) {
        return FontHelper.load("NotoSansSC-Regular.ttf", size);
    }

    public static Font getMedium(float size) {
        return FontHelper.load("NotoSansSC-Medium.ttf", size);
    }

    public static Font getBold(float size) {
        return FontHelper.load("NotoSansSC-Bold.ttf", size);
    }

    public static Font getBorel(float size) {
        return FontHelper.load("Borel-Regular.ttf", size);
    }

    public static Font getUrbanistVariable(float size) {
        return FontHelper.load("Urbanist-VariableFont_wght.ttf", size);
    }

    public static Font getInterBold(float size) {
        return FontHelper.load("Inter-Bold.ttf", size);
    }

    public static Font getInterMedium(float size) {
        return FontHelper.load("Inter-Medium.ttf", size);
    }

    public static Font getInterRegular(float size) {
        return FontHelper.load("Inter-Regular.ttf", size);
    }

    public static Font getInterLight(float size) {
        return FontHelper.load("Inter-Light.ttf", size);
    }

    public static Font getIconFill(float size) {
        return FontHelper.load("MaterialSymbolsRounded_Fill.ttf", size);
    }

    public static Font getIcon(float size) {
        return FontHelper.load("MaterialSymbolsRounded.ttf", size);
    }
}
