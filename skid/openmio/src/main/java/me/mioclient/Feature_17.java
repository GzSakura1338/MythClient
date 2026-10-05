package me.mioclient;

import java.awt.Desktop;
import java.io.IOException;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_17.class */
public final class Feature_17 extends Feature {
    public Feature_17() {
        super("folder");
        do414("openfolder");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.executes(commandContext -> {
            try {
                Desktop.getDesktop().open(PresetHelper.path.toFile());
                return 1;
            } catch (Exception e) {
                e.printStackTrace();
                return 1;
            }
        });
    }
}
