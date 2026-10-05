package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import com.mojang.brigadier.builder.ArgumentBuilder;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_8.class */
public final class Feature_8 extends Feature {

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/Feature_8$Mode.class */
    public enum Mode {
        MODULE(new ArgumentType_8()),
        PLAYER(new ExamplesArgumentType_3());

        public final com.mojang.brigadier.arguments.ArgumentType<?> argumentType;

        Mode(com.mojang.brigadier.arguments.ArgumentType argumentType) {
            this.argumentType = argumentType;
        }
    }

    public Feature_8() {
        super("alias");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(getArgumentBuilder1272(Mode.MODULE)).then(getArgumentBuilder1272(Mode.PLAYER));
    }

    public ArgumentBuilder<CommandSource, ?> getArgumentBuilder1272(Mode mode) {
        return Feature.literal(mode.name().toLowerCase()).then(Feature.literal("set").then(Feature.argument("target", mode.argumentType).then(Feature.argument("alias", StringArgumentType.string()).executes(commandContext -> {
            String str = (String) commandContext.getArgument("alias", String.class);
            if (mode == Mode.MODULE) {
                BaritoneHelper_3.notificationsHelper.do393((Module) commandContext.getArgument("target", Module.class), str);
                return 1;
            }
            BaritoneHelper_3.notificationsHelper.do394((String) commandContext.getArgument("target", String.class), str);
            return 1;
        })))).then(Feature.literal("remove").then(Feature.argument("target", mode.argumentType).executes(commandContext2 -> {
            if (mode == Mode.MODULE) {
                BaritoneHelper_3.notificationsHelper.do395((Module) commandContext2.getArgument("target", Module.class));
                return 1;
            }
            BaritoneHelper_3.notificationsHelper.do396((String) commandContext2.getArgument("target", String.class));
            return 1;
        })));
    }
}
