package me.mioclient.module;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Objects;
import java.util.stream.Stream;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.BooleanSetting;
import me.mioclient.EnumSetting;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.ModuleListCryptoHelper;
import me.mioclient.ModuleListMode;
import me.mioclient.ModuleListSearchHelper4_2;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.module.client.HUD;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/ModuleList.class */
public class ModuleList extends me.mioclient.ModuleList {
    public Setting<Boolean> setting;
    public Setting<HUD.ModuleListMode> setting2;
    public final List<ModuleListCryptoHelper> list;

    public ModuleList() {
        super("ModuleList", "arraylist");
        this.setting = add(new BooleanSetting("OnlyBound", false));
        this.setting2 = add(new EnumSetting("Sort", HUD.ModuleListMode.LENGTH));
        this.list = new ArrayList();
        do3019(new ModuleListSearchHelper4_2(this, this.list));
        getModuleListSearchHelper43020().do2952(ModuleListMode.TOP_RIGHT);
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        this.list.clear();
        Stream map = BaritoneHelper_3.keyPearlSearchHelper4.getRegistry().stream().map(module -> {
            return new ModuleListCryptoHelper(this, module);
        });
        List<ModuleListCryptoHelper> list = this.list;
        Objects.requireNonNull(list);
        map.forEach((v1) -> {
            list.add((ModuleListCryptoHelper) v1);
        });
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (this.setting2.getValue() == HUD.ModuleListMode.ALPHABET) {
            this.list.sort(Comparator.comparing(moduleListCryptoHelper -> {
                return moduleListCryptoHelper.getModule595().getInfoString();
            }));
        } else {
            this.list.sort(Comparator.comparingDouble(moduleListCryptoHelper2 -> {
                return -FontsSearchHelper4.fontsSearchHelper4.get1316(moduleListCryptoHelper2.getModule595().getInfoString());
            }));
        }
    }
}
