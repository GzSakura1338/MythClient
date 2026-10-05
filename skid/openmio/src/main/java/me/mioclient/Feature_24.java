package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_24.class */
public class Feature_24 extends Feature {
    public Feature_24() {
        super("kit");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.getRequiredArgumentBuilder411("save", "add").then(Feature.argument("name", StringArgumentType.string()).executes(commandContext -> {
            String str = (String) commandContext.getArgument("name", String.class);
            BaritoneHelper_3.chestStealerSearchHelper4_3.do40(str);
            MixinMessageIndicatorHelper.do344(Text.literal("Kit %s has been saved".formatted(str)), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })));
        literalArgumentBuilder.then(Feature.getRequiredArgumentBuilder411("remove", "delete", "del").then(Feature.argument("kit", new ArgumentType_2()).executes(commandContext2 -> {
            ChestStealerEnumSettingHelper chestStealerEnumSettingHelper = (ChestStealerEnumSettingHelper) commandContext2.getArgument("kit", ChestStealerEnumSettingHelper.class);
            BaritoneHelper_3.chestStealerSearchHelper4_3.getRegistry().removeIf(chestStealerEnumSettingHelper2 -> {
                return chestStealerEnumSettingHelper.getName().equalsIgnoreCase(chestStealerEnumSettingHelper2.getName());
            });
            MixinMessageIndicatorHelper.do344(Text.literal("Kit %s has been removed".formatted(chestStealerEnumSettingHelper.getName())), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })));
        literalArgumentBuilder.then(Feature.literal("load").then(Feature.argument("kit", new ArgumentType_2()).executes(commandContext3 -> {
            ChestStealerEnumSettingHelper chestStealerEnumSettingHelper = (ChestStealerEnumSettingHelper) commandContext3.getArgument("kit", ChestStealerEnumSettingHelper.class);
            BaritoneHelper_3.chestStealerSearchHelper4_3.do258(chestStealerEnumSettingHelper.getName());
            MixinMessageIndicatorHelper.do344(Text.literal("Kit %s has been loaded".formatted(chestStealerEnumSettingHelper.getName())), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })));
    }
}
