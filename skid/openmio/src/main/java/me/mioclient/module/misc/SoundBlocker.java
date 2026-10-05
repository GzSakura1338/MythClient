package me.mioclient.module.misc;

import java.util.Iterator;
import java.util.Set;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.PlayEvent;
import me.mioclient.module.Module;
import net.minecraft.sound.SoundEvent;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/SoundBlocker.class */
public class SoundBlocker extends Module {
    public Setting<Set<SoundEvent>> sounds;

    public SoundBlocker() {
        super("SoundBlocker", "Blocks certain sounds from being played.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void onPlay(PlayEvent playEvent) {
        Iterator<SoundEvent> it = this.sounds.getValue().iterator();
        while (it.hasNext()) {
            if (it.next().getId().equals(playEvent.getSoundInstance1914().getId())) {
                playEvent.do1162();
                return;
            }
        }
    }
}
