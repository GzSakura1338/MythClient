package me.mioclient;

import me.mioclient.mixin.ducks.DuckHandledScreen;
import me.mioclient.module.player.ChestStealer;
import net.fabricmc.fabric.api.client.screen.v1.ScreenEvents;
import net.fabricmc.fabric.api.client.screen.v1.Screens;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.Element;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.ingame.GenericContainerScreen;
import net.minecraft.client.gui.screen.ingame.ShulkerBoxScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.text.Text;

/* JADX INFO: Access modifiers changed from: package-private */
/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChestStealerSearchHelper4.class */
public class ChestStealerSearchHelper4 implements SearchHelper_4, ScreenEvents.AfterInit {
    public final ChestStealer chestStealer;

    public ChestStealerSearchHelper4(ChestStealer chestStealer) {
        this.chestStealer = chestStealer;
    }

    public void afterInit(MinecraftClient minecraftClient, Screen screen, int i, int i2) {
        if (((screen instanceof GenericContainerScreen) || (screen instanceof ShulkerBoxScreen)) && this.chestStealer.isToggled() && this.chestStealer.onlyButtons.getValue().booleanValue()) {
            this.chestStealer.do2361((ChestStealerMode) null);
            int i3 = FreecamHelper.num;
            int bgWidth = (((screen.width - ((DuckHandledScreen) screen).getBgWidth()) / 2) - i3) - 2;
            int bgHeight = (screen.height - ((DuckHandledScreen) screen).getBgHeight()) / 2;
            for (ChestStealerMode chestStealerMode : ChestStealerMode.values()) {
                ButtonWidget.Builder builder = new ButtonWidget.Builder(Text.literal(chestStealerMode.getName()), buttonWidget -> {
                    if (chestStealerMode != this.chestStealer.getChestStealerMode2362()) {
                        this.chestStealer.do2361(chestStealerMode);
                        return;
                    }
                    BaritoneHelper_3.tooltipsSearchHelper4_2.do164(() -> {
                        screen.setFocused((Element) null);
                    }, 0);
                    this.chestStealer.do2361((ChestStealerMode) null);
                });
                builder.dimensions(bgWidth, bgHeight, i3, 15);
                bgHeight += 17;
                Screens.getButtons(screen).add(builder.build());
            }
        }
    }
}
