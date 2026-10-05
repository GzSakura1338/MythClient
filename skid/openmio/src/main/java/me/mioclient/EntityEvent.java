package me.mioclient;

import java.awt.Color;
import me.mioclient.event.Event;
import net.minecraft.entity.Entity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/EntityEvent.class */
public class EntityEvent extends Event {
    public static final EntityEvent entityEvent = new EntityEvent();
    public Entity entity;
    public int num;
    public float val;
    public float val2;
    public float val3;
    public float val4;

    public static EntityEvent getEntityEvent180(Entity entity, int i, float f, float f2, float f3, float f4) {
        entityEvent.num = i;
        entityEvent.entity = entity;
        entityEvent.val = f;
        entityEvent.val2 = f2;
        entityEvent.val3 = f3;
        entityEvent.val4 = f4;
        entityEvent.do2402();
        return entityEvent;
    }

    public Entity getEntity181() {
        return this.entity;
    }

    public void do182(Entity entity) {
        this.entity = entity;
    }

    public float get183() {
        return this.val;
    }

    public void do184(float f) {
        this.val = f;
    }

    public float get185() {
        return this.val2;
    }

    public void do186(float f) {
        this.val2 = f;
    }

    public float get187() {
        return this.val3;
    }

    public void do188(float f) {
        this.val3 = f;
    }

    public float get189() {
        return this.val4;
    }

    public void do190(float f) {
        this.val4 = f;
    }

    public int get191() {
        return this.num;
    }

    public void do192(int i) {
        this.num = i;
    }

    public void do193(Color color) {
        do184(color.getRed() / 255.0f);
        do186(color.getGreen() / 255.0f);
        do188(color.getBlue() / 255.0f);
        do190(color.getAlpha() / 255.0f);
    }
}
