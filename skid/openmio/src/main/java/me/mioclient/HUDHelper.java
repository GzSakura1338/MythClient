package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HUDHelper.class */
public interface HUDHelper {
    boolean isToggled();

    default void do495(boolean z) {
        if (isToggled() != z) {
            do496();
        }
    }

    default void do496() {
        if (isToggled()) {
            disable();
        } else {
            enable();
        }
    }

    void enable();

    void disable();
}
