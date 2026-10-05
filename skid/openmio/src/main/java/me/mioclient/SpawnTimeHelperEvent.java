package me.mioclient;

import me.mioclient.event.Event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpawnTimeHelperEvent.class */
public class SpawnTimeHelperEvent extends Event {
    public final SpawnTimeHelper spawnTimeHelper;

    public SpawnTimeHelperEvent(SpawnTimeHelper spawnTimeHelper) {
        this.spawnTimeHelper = spawnTimeHelper;
    }

    public SpawnTimeHelper getSpawnTimeHelper1787() {
        return this.spawnTimeHelper;
    }
}
