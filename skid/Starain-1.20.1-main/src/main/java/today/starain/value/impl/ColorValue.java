package today.starain.value.impl;

import today.starain.module.ClientModule;
import today.starain.value.Value;

import java.awt.*;
import java.util.function.Supplier;

public final class ColorValue extends Value {
    private Color value;

    public ColorValue(String name, Color value, ClientModule parent, Supplier<Boolean> visible) {
        super(name, parent, visible);
        this.value = value;
    }

    public ColorValue(String name, Color value, ClientModule parent) {
        this(name, value, parent, () -> true);
    }

    public Color get() {
        return value;
    }

    public void set(Color value) {
        this.value = value;
    }

    public int getArgb() {
        Color color = value == null ? new Color(255, 255, 255, 255) : value;
        return ((color.getAlpha() & 255) << 24)
                | ((color.getRed() & 255) << 16)
                | ((color.getGreen() & 255) << 8)
                | (color.getBlue() & 255);
    }

    public void setArgb(int argb) {
        int a = (argb >>> 24) & 255;
        int r = (argb >>> 16) & 255;
        int g = (argb >>> 8) & 255;
        int b = argb & 255;
        value = new Color(r, g, b, a);
    }
}
