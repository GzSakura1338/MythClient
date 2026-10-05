package me.mioclient.module;

import java.awt.Color;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Set;
import me.mioclient.BooleanSetting;
import me.mioclient.CryptoHelper;
import me.mioclient.EffectsSearchHelper4;
import me.mioclient.EntityListObjectSetting;
import me.mioclient.EnumSetting;
import me.mioclient.ModuleListMode;
import me.mioclient.ModuleListSearchHelper4_2;
import me.mioclient.ScaffoldMode_2;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import net.minecraft.entity.effect.StatusEffect;
import net.minecraft.registry.Registries;
import net.minecraft.registry.entry.RegistryEntry;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Effects.class */
public class Effects extends me.mioclient.ModuleList {
    public Setting<Boolean> setting;
    public Setting<ScaffoldMode_2> setting2;
    public Setting<Set<StatusEffect>> setting3;
    public final List<EffectsSearchHelper4> list;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/Effects$Inner.class */
    class Inner extends ModuleListSearchHelper4_2 {
        public Inner(me.mioclient.ModuleList moduleList, List list) {
            super(moduleList, (List<? extends CryptoHelper>) list);
        }

        @Override // me.mioclient.ModuleListSearchHelper4_2
        public Color getColor489(float f, CryptoHelper cryptoHelper) {
            return Effects.this.setting.getValue().booleanValue() ? new Color(((StatusEffect) ((EffectsSearchHelper4) cryptoHelper).getRegistryEntry830().value()).getColor(), false) : super.getColor489(f, cryptoHelper);
        }
    }

    public Effects() {
        super("Effects", "potions");
        this.setting = add(new BooleanSetting("Vanilla", false));
        this.setting2 = add(new EnumSetting("Selection", ScaffoldMode_2.ANY));
        this.setting3 = add(new EntityListObjectSetting("WhiteList", Registries.STATUS_EFFECT, new StatusEffect[0]));
        this.list = new ArrayList();
        do3019(new Inner(this, this.list));
        getModuleListSearchHelper43020().do2952(ModuleListMode.BOTTOM_RIGHT);
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        this.list.clear();
        Registries.STATUS_EFFECT.streamEntries().forEach(reference -> {
            this.list.add(new EffectsSearchHelper4(reference, () -> {
                boolean z;
                if (minecraftClient.player.hasStatusEffect((RegistryEntry) reference)) {
                    if (this.setting2.getValue().is1392((StatusEffect) reference.value(), this.setting3)) {
                        z = true;
                        return Boolean.valueOf(z);
                    }
                }
                z = false;
                return Boolean.valueOf(z);
            }));
        });
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        this.list.sort(Comparator.comparing(effectsSearchHelper4 -> {
            return ((StatusEffect) effectsSearchHelper4.getRegistryEntry830().value()).getName().getString();
        }));
    }
}
