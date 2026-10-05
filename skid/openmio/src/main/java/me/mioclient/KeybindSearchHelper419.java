package me.mioclient;

import java.util.function.Function;
import me.mioclient.api.Keybind;
import me.mioclient.api.Setting;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/KeybindSearchHelper419.class */
public class KeybindSearchHelper419 extends SearchHelper4_19 {
    public final ArrayListPresetHelper2 arrayListPresetHelper2;
    public Setting<Keybind> setting;
    public boolean flag;

    public KeybindSearchHelper419(int i, ArrayListPresetHelper2 arrayListPresetHelper2) {
        super(arrayListPresetHelper2.getPresetEnumSettingHelper1394(), i);
        this.arrayListPresetHelper2 = arrayListPresetHelper2;
        this.setting = null;
    }

    public KeybindSearchHelper419(int i, ArrayListPresetHelper2 arrayListPresetHelper2, Setting<Keybind> setting) {
        super(arrayListPresetHelper2.getPresetEnumSettingHelper1394(), i);
        this.arrayListPresetHelper2 = arrayListPresetHelper2;
        this.setting = setting;
    }

    @Override // me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
    public void do20(double d, double d2, int i) {
        if (this.arrayListPresetHelper2.flag) {
            super.do20(d, d2, i);
            if (is92(d, d2) && this.arrayListPresetHelper2.flag) {
                if (i == 0) {
                    this.flag = !this.flag;
                    return;
                }
                if (this.flag) {
                    modifyKeybind(keybind -> {
                        return keybind.getKeybind1941(i).getKeybind1943(true);
                    });
                    this.flag = !this.flag;
                } else if (i == 1) {
                    modifyKeybind(keybind2 -> {
                        return keybind2.getKeybind1942(Keybind.KeybindMode.values()[(keybind2.getKeybindMode1946().ordinal() + 1) % Keybind.KeybindMode.values().length]);
                    });
                }
            }
        }
    }

    @Override // me.mioclient.PresetHelper_5
    public void do89(int i) {
        if (this.flag) {
            boolean z = i == 261 || i == 256;
            modifyKeybind(keybind -> {
                return keybind.getKeybind1941(z ? -1 : i).getKeybind1943(false);
            });
            this.flag = false;
            BaritoneHelper_3.getMixinTitleScreenSearchHelper42216().do2837();
        }
    }

    @Override // me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
    public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
        super.do19(drawContext, matrixStack, d, d2);
        if (this.flag) {
            FontsSearchHelper4_2.mode_5 = Mode_5.INPUT;
        }
        String name = this.flag ? getKeybind().getKeybindMode1946().getName() : "Key";
        String string773 = getKeybind().getString773();
        if (this.flag) {
            string773 = "...";
        }
        FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, new ArgumentTypeHelper().getArgumentTypeHelper2919(string773).getArgumentTypeHelper2919(name).getString2921("\u0001 \u0001"), this.presetEnumSettingHelper.getX() + 4, ((this.presetEnumSettingHelper.getY() + get1742()) - get1396()) + this.num, getUI1744().textColor.getValue());
    }

    public void modifyKeybind(Function<Keybind, Keybind> function) {
        if (this.setting == null) {
            this.arrayListPresetHelper2.module.modifyKeybind(function);
        } else {
            this.setting.do2333(function.apply(this.setting.getValue()));
        }
    }

    public Keybind getKeybind() {
        return this.setting == null ? this.arrayListPresetHelper2.module.getKeybind() : this.setting.getValue();
    }
}
