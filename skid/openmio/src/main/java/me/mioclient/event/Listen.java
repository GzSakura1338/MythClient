package me.mioclient.event;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/* compiled from: 0.java */
@Target({ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
/* loaded from: mio-yarn.jar:me/mioclient/event/Listen.class */
public @interface Listen {
    int get219() default 0;
}
