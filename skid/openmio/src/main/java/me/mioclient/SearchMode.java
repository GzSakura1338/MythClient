package me.mioclient;

import com.mojang.blaze3d.systems.RenderSystem;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchMode.class */
public enum SearchMode implements EnumSettingHelper {
    NORMAL("Normal"),
    PROTANOPIA("Protanopia") { // from class: me.mioclient.SearchMode.Inner
        @Override // me.mioclient.SearchMode
        public float[] getFloatArray1028() {
            return new float[]{0.3f, 0.2f, 1.0f, 1.0f};
        }

        @Override // me.mioclient.SearchMode
        public void do1027(java.lang.Runnable runnable) {
            float[] floatArray1028 = getFloatArray1028();
            RenderSystem.setShaderColor(floatArray1028[0], floatArray1028[1], floatArray1028[2], floatArray1028[3]);
            runnable.run();
            RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
        }
    },
    DEUTERANOPIA("Deuteranopia") { // from class: me.mioclient.SearchMode.Inner_3
        @Override // me.mioclient.SearchMode
        public float[] getFloatArray1028() {
            return new float[]{0.3f, 0.0f, 1.0f, 1.0f};
        }

        @Override // me.mioclient.SearchMode
        public void do1027(java.lang.Runnable runnable) {
            float[] floatArray1028 = getFloatArray1028();
            RenderSystem.setShaderColor(floatArray1028[0], floatArray1028[1], floatArray1028[2], floatArray1028[3]);
            runnable.run();
            RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
        }
    },
    TRITANOPIA("Tritanopia") { // from class: me.mioclient.SearchMode.Inner_2
        @Override // me.mioclient.SearchMode
        public float[] getFloatArray1028() {
            return new float[]{1.0f, 0.7f, 0.0f, 1.0f};
        }

        @Override // me.mioclient.SearchMode
        public void do1027(java.lang.Runnable runnable) {
            float[] floatArray1028 = getFloatArray1028();
            RenderSystem.setShaderColor(floatArray1028[0], floatArray1028[1], floatArray1028[2], floatArray1028[3]);
            runnable.run();
            RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
        }
    };

    public final String name;

    SearchMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public void do1027(java.lang.Runnable runnable) {
        runnable.run();
    }

    public float[] getFloatArray1028() {
        return new float[]{1.0f, 1.0f, 1.0f, 1.0f};
    }
}
