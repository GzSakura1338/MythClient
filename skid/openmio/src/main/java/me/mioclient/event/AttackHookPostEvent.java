package me.mioclient.event;

import me.mioclient.KeyPearlMode;
import net.minecraft.entity.Entity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/AttackHookPostEvent.class */
public class AttackHookPostEvent extends Event {
    public final KeyPearlMode keyPearlMode;
    public Entity entity;

    public AttackHookPostEvent(KeyPearlMode keyPearlMode, Entity entity) {
        this.keyPearlMode = keyPearlMode;
        this.entity = entity;
    }

    public Entity getEntity181() {
        return this.entity;
    }

    public KeyPearlMode getKeyPearlMode1472() {
        return this.keyPearlMode;
    }
}
