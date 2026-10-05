package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelper_2.class */
public interface PresetHelper_2 {
    public static final PresetHelper_2 presetHelper_2 = () -> {
        return false;
    };

    boolean isClosed();

    default boolean is623() {
        return !isClosed();
    }
}
