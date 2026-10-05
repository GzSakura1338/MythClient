package me.mioclient;

import net.minecraft.entity.passive.AnimalEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoBreedHelper.class */
public final class AutoBreedHelper {
    public final AnimalEntity animalEntity;
    public int num = 0;

    public AutoBreedHelper(AnimalEntity animalEntity) {
        this.animalEntity = animalEntity;
    }

    public void do1457() {
        this.num++;
    }

    public AnimalEntity getAnimalEntity1458() {
        return this.animalEntity;
    }

    public int get1459() {
        return this.num;
    }
}
