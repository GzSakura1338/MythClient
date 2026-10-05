package me.mioclient;

import java.util.Iterator;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.client.gui.screen.ConfirmScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_21.class */
public class Feature_21 extends Feature {
    public Feature_21() {
        super("defaults");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.executes(commandContext -> {
            ConfirmScreen confirmScreen = new ConfirmScreen(z -> {
                if (z) {
                    for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
                        Iterator<Setting<?>> it = module.getRegistry().iterator();
                        while (it.hasNext()) {
                            it.next().reset();
                        }
                        module.do495(false);
                    }
                }
                minecraftClient.setScreen((Screen) null);
            }, Text.of("Reset to defaults."), Text.of("Continue?"));
            do413(() -> {
                minecraftClient.setScreen((Screen) confirmScreen);
            });
            return 1;
        });
    }
}
