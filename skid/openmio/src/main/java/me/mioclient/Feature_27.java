package me.mioclient;

import java.util.Iterator;
import me.mioclient.api.Keybind;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_27.class */
public final class Feature_27 extends Feature {
    public Feature_27() {
        super("bind");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("set").then(Feature.argument("module", new ArgumentType_8()).then(Feature.argument("key", new ArgumentType_4()).executes(commandContext -> {
            Module module = (Module) commandContext.getArgument("module", Module.class);
            Keybind keybind = (Keybind) commandContext.getArgument("key", Keybind.class);
            module.modifyKeybind(keybind2 -> {
                return keybind2.getKeybind1941(keybind.get1945()).getKeybind1943(keybind.is1947());
            });
            MixinMessageIndicatorHelper.do344(Text.literal("Bind for ").append(getMutableText341(module.getName())).append(" has been set to ").append(getMutableText341(EntityControlSearchHelper4.getString2603(keybind.get1945(), keybind.is1947()))), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })))).then(Feature.literal("clear").then(Feature.argument("module", new ArgumentType_8()).executes(commandContext2 -> {
            Module module = (Module) commandContext2.getArgument("module", Module.class);
            module.modifyKeybind(keybind -> {
                return keybind.getKeybind1941(-1);
            });
            MixinMessageIndicatorHelper.do344(Text.literal("Bind for ").append(getMutableText341(module.getName())).append(" has been reset"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })).then(Feature.literal("all").executes(commandContext3 -> {
            Iterator it = BaritoneHelper_3.keyPearlSearchHelper4.getRegistry().iterator();
            while (it.hasNext()) {
                ((Module) it.next()).modifyKeybind(keybind -> {
                    return keybind.getKeybind1941(-1);
                });
            }
            MixinMessageIndicatorHelper.do344(Text.literal("Bind for all modules has been reset"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })));
    }
}
