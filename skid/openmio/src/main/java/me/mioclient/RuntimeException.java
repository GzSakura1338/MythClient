package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/RuntimeException.class */
public class RuntimeException extends java.lang.RuntimeException {
    public RuntimeException(Class<?> cls) {
        super("No registered lambda listener for '" + cls.getName() + "'.");
    }
}
