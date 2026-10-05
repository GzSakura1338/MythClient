package me.mioclient.module.render;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.Helper_7;
import me.mioclient.SearchHelper_2;
import me.mioclient.api.Category;
import me.mioclient.event.Listen;
import me.mioclient.event.TickPostEvent;
import me.mioclient.module.Module;
import me.mioclient.module.player.Freecam;
import net.minecraft.client.option.Perspective;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/render/FreeLook.class */
public class FreeLook extends Module {
    public static Freecam freecam = (Freecam) BaritoneHelper_3.baritoneHelper_4.getModule117(Freecam.class);
    public Perspective perspective;

    public FreeLook() {
        super("FreeLook", "Allows you to rotate your 3rd person camera without rotating your player.", Category.RENDER, new String[0]);
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        if (freecam.isToggled()) {
            return;
        }
        this.perspective = minecraftClient.options.getPerspective();
        minecraftClient.options.setPerspective(getPerspective2612());
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        if (this.perspective == Perspective.FIRST_PERSON) {
            minecraftClient.options.setPerspective(Perspective.FIRST_PERSON);
        }
    }

    @Listen(get219= Helper_7.num5)
    public void do32(TickPostEvent tickPostEvent) {
        if (freecam.isToggled()) {
            return;
        }
        minecraftClient.options.setPerspective(getPerspective2612());
        minecraftClient.crosshairTarget = minecraftClient.player.raycast(minecraftClient.player.getBlockInteractionRange(), SearchHelper_2.get536(), false);
    }

    public Perspective getPerspective2612() {
        if (this.perspective == Perspective.FIRST_PERSON) {
            return Perspective.THIRD_PERSON_BACK;
        }
        if (this.perspective == null) {
            this.perspective = Perspective.FIRST_PERSON;
        }
        return this.perspective;
    }
}
