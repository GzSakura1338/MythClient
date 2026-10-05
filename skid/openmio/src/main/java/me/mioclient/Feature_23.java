package me.mioclient;

import com.mojang.brigadier.arguments.BoolArgumentType;
import java.util.Iterator;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_23.class */
public final class Feature_23 extends Feature {
    public Feature_23() {
        super("drawn");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("module", new ArgumentType_8()).executes(commandContext -> {
            Module module = (Module) commandContext.getArgument("module", Module.class);
            module.setDrawn(!module.isDrawn());
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2902(module.isDrawn()).getArgumentTypeHelper2919(module.getName()).getString2921("Drawing for \u0001 has been set to \u0001.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })).then(Feature.literal("all").then(Feature.argument("state", BoolArgumentType.bool()).executes(commandContext2 -> {
            boolean booleanValue = ((Boolean) commandContext2.getArgument("state", Boolean.class)).booleanValue();
            Iterator it = BaritoneHelper_3.keyPearlSearchHelper4.getRegistry().iterator();
            while (it.hasNext()) {
                ((Module) it.next()).setDrawn(booleanValue);
            }
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2902(booleanValue).getString2921("Drawing for all modules has been set to \u0001.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })));
    }
}
