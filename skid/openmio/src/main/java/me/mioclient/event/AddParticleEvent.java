package me.mioclient.event;

import net.minecraft.client.particle.Particle;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/AddParticleEvent.class */
public class AddParticleEvent extends Event {
    public Particle particle;

    public AddParticleEvent(Particle particle) {
        this.particle = particle;
    }

    public Particle getParticle982() {
        return this.particle;
    }

    public void do983(Particle particle) {
        this.particle = particle;
    }
}
