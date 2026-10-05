package me.mioclient.module.misc;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.Helper_7;
import me.mioclient.KeyPearlMode;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchIdentifier;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.AddMessageEvent;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/PMSound.class */
public class PMSound extends Module {
    public Setting<SearchIdentifier> type;
    public Setting<Float> volume;

    public PMSound() {
        super("PMSound", "Plays a sound whenever you get a private message.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen(get219= Helper_7.num2)
    public void onAddMessage(AddMessageEvent addMessageEvent) {
        if (!addMessageEvent.is2403() && addMessageEvent.getKeyPearlMode1472() == KeyPearlMode.Pre && addMessageEvent.getText2279() != null && MixinMessageIndicatorHelper.is335(addMessageEvent.getText2279().getString())) {
            minecraftClient.executeSync(() -> {
                BaritoneHelper_3.searchHelper4_11.getSearchHelper4_42970(this.type.getValue()).do1820(this.volume.getValue().floatValue());
            });
        }
    }
}
