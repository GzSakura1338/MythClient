package me.mioclient;

import java.awt.Color;
import me.mioclient.module.client.UI;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MixinMessageIndicatorHelper_2.class */
public class MixinMessageIndicatorHelper_2 {
    public static final Color color = new Color(0, 0, 0, 0);

    public static Color getColor811() {
        return UI.uI.color.getValue();
    }

    public static Color getColor812(int i, float f, float f2, int i2) {
        return getColor813(FreecamHelper.num3, i, f, f2, i2);
    }

    public static Color getColor813(int i, int i2, float f, float f2, int i3) {
        return getColor816(Color.getHSBColor((float) ((Math.ceil((System.currentTimeMillis() + i2) / 20.0d) % i) / i), f, f2), i3);
    }

    public static Color getColor814(Color color2, Color color3, double d, double d2) {
        double currentTimeMillis = ((System.currentTimeMillis() + d2) % d) / d;
        return currentTimeMillis > FreecamHelper.val2 ? getColor815(color3, color2, (float) ((currentTimeMillis - FreecamHelper.val2) * 2.0d)) : getColor815(color2, color3, (float) (currentTimeMillis * 2.0d));
    }

    public static Color getColor815(Color color2, Color color3, float f) {
        try {
            return new Color((int) ((color2.getRed() * f) + (color3.getRed() * (1.0f - f))), (int) ((color2.getGreen() * f) + (color3.getGreen() * (1.0f - f))), (int) ((color2.getBlue() * f) + (color3.getBlue() * (1.0f - f))), (int) ((color2.getAlpha() * f) + (color3.getAlpha() * (1.0f - f))));
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            return Color.white;
        }
    }

    public static Color getColor816(Color color2, int i) {
        return new Color(color2.getRed(), color2.getGreen(), color2.getBlue(), i);
    }

    public static Color getColor817(Color color2, float f) {
        return getColor816(color2, Math.round(f * 255.0f));
    }

    public static int get818(Color color2, int i) {
        return get821(color2.getRed(), color2.getGreen(), color2.getBlue(), i);
    }

    public static int get819(Color color2, float f) {
        return get821(color2.getRed(), color2.getGreen(), color2.getBlue(), (int) (f * 255.0f));
    }

    public static Color getColor820(Color color2) {
        return getColor816(color2, 0);
    }

    public static int get821(int i, int i2, int i3, int i4) {
        return ((i4 & 255) << 24) | ((i & 255) << 16) | ((i2 & 255) << 8) | (i3 & 255);
    }

    public static int get822(float f, float f2, float f3, float f4) {
        return ((((int) (f4 * 255.0f)) & 255) << 24) | ((((int) (f * 255.0f)) & 255) << 16) | ((((int) (f2 * 255.0f)) & 255) << 8) | (((int) (f3 * 255.0f)) & 255);
    }

    public static Color getColor823(int i) {
        return new Color((i >> 16) & 255, (i >> 8) & 255, i & 255, (i >> 24) & 255);
    }

    public static Color getColor824(Color color2, float f) {
        return new Color(Math.max((int) (color2.getRed() * f), 0), Math.max((int) (color2.getGreen() * f), 0), Math.max((int) (color2.getBlue() * f), 0), color2.getAlpha());
    }

    public static Color getColor825(Color color2, float f) {
        int red = color2.getRed();
        int green = color2.getGreen();
        int blue = color2.getBlue();
        int alpha = color2.getAlpha();
        int i = (int) (1.0d / (1.0d - f));
        if (red == 0 && green == 0 && blue == 0) {
            return new Color(i, i, i, alpha);
        }
        if (red > 0 && red < i) {
            red = i;
        }
        if (green > 0 && green < i) {
            green = i;
        }
        if (blue > 0 && blue < i) {
            blue = i;
        }
        return new Color(Math.min((int) (red / f), 255), Math.min((int) (green / f), 255), Math.min((int) (blue / f), 255), alpha);
    }

    public static String getString826(Color color2, boolean z) {
        StringBuilder sb = new StringBuilder();
        sb.append(getString827(color2.getAlpha()));
        sb.append(getString827(color2.getRed()));
        sb.append(getString827(color2.getGreen()));
        sb.append(getString827(color2.getBlue()));
        return (z ? "#" : "") + String.valueOf(sb);
    }

    public static String getString827(int i) {
        return (i < 16 ? "0" : "") + Integer.toHexString(i);
    }

    public static Color getColor828(String str) {
        if (str.length() < 6) {
            throw new IllegalArgumentException();
        }
        if (str.startsWith("#")) {
            str = str.substring(1);
        }
        boolean z = str.length() == 8;
        int[] iArr = new int[z ? 4 : 3];
        int i = 0;
        while (true) {
            if (i >= (z ? 4 : 3)) {
                break;
            }
            iArr[i] = Integer.parseInt(str.substring(i * 2, (i + 1) * 2), 16);
            i++;
        }
        int i2 = z ? 1 : 0;
        return new Color(iArr[i2], iArr[1 + i2], iArr[2 + i2], z ? iArr[0] : 255);
    }
}
