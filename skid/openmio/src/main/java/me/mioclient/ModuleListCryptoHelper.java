package me.mioclient;

import me.mioclient.module.Module;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ModuleListCryptoHelper.class */
public class ModuleListCryptoHelper extends CryptoHelper {
    public final Module module;

    public ModuleListCryptoHelper(me.mioclient.module.ModuleList moduleList, Module module) {
        super(() -> {
            return Text.literal(module.getInfoString());
        }, () -> {
            return Boolean.valueOf(module.isToggled() && module.isDrawn() && !(module.getKeybind().is1944() && moduleList.setting.getValue().booleanValue()));
        });
        this.module = module;
    }

    public Module getModule595() {
        return this.module;
    }
}
