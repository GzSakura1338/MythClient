package me.mioclient;

import me.mioclient.module.client.UI;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelper_5.class */
public interface PresetHelper_5 {
    default void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
    }

    default void do91(double d, double d2) {
    }

    default void do20(double d, double d2, int i) {
    }

    default void do88(double d, double d2, int i) {
    }

    default void do654(double d, double d2, double d3) {
    }

    default void do89(int i) {
    }

    default void do90(char c) {
    }

    default void do653(int i) {
    }

    default int get1395() {
        return 0;
    }

    default void init() {
    }

    default int get93() {
        return get1743();
    }

    default boolean is92(double d, double d2) {
        return false;
    }

    default float get1742() {
        return ((get1743() - FontsSearchHelper4.fontsSearchHelper4.get93()) + 1) / 2.0f;
    }

    default int get1743() {
        return 2 + FontsSearchHelper4.fontsSearchHelper4.get93() + UI.uI.buttonHeight.getValue().intValue();
    }

    default UI getUI1744() {
        return UI.uI;
    }
}
