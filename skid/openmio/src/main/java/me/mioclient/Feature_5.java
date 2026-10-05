package me.mioclient;

import java.util.HashSet;
import java.util.Set;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_5.class */
public final class Feature_5 extends Feature {
    public final Set<Module> set;

    public Feature_5() {
        super("alloff");
        this.set = new HashSet();
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.executes(commandContext -> {
            this.set.clear();
            for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
                if (!(module instanceof ModuleList) && module.isToggled()) {
                    module.do496();
                    this.set.add(module);
                }
            }
            return 1;
        });
        literalArgumentBuilder.then(Feature.getRequiredArgumentBuilder411("restore", "backup").executes(commandContext2 -> {
            for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
                if (!(module instanceof ModuleList)) {
                    module.do495(this.set.contains(module));
                }
            }
            return 1;
        }));
    }
}
