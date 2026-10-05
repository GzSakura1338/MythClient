package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import java.util.Iterator;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.module.misc.ChatFilter;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_34.class */
public class Feature_34 extends Feature {
    public static ChatFilter chatFilter = (ChatFilter) BaritoneHelper_3.baritoneHelper_4.getModule117(ChatFilter.class);

    public Feature_34() {
        super("chatfilter");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("add").then(Feature.argument("id", StringArgumentType.string()).then(Feature.argument("filter", StringArgumentType.string()).executes(commandContext -> {
            String str = (String) commandContext.getArgument("id", String.class);
            String str2 = (String) commandContext.getArgument("filter", String.class);
            if (BaritoneHelper_3.chatFilterSearchHelper4.is2675(str)) {
                MixinMessageIndicatorHelper.do345(Text.of("Chat filter \"%s\" already exists.".formatted(str)), MixinMessageIndicatorHelper.getMessageSignatureData337(-3469856), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
                return 1;
            }
            BaritoneHelper_3.chatFilterSearchHelper4.do2676(str, str2);
            MixinMessageIndicatorHelper.do344(Text.of("Chat filter \"%s\" has been added.".formatted(str)), MixinMessageIndicatorHelper.getMessageSignatureData337(-3469857));
            return 1;
        })))).then(Feature.literal("remove").then(Feature.argument("id", new ArgumentType_12()).executes(commandContext2 -> {
            Data data = (Data) commandContext2.getArgument("id", Data.class);
            BaritoneHelper_3.chatFilterSearchHelper4.do2680(data);
            MixinMessageIndicatorHelper.do344(Text.of("Chat filter \"%s\" has been removed.".formatted(data.getString1093())), MixinMessageIndicatorHelper.getMessageSignatureData337(-3469858));
            return 1;
        }))).then(Feature.literal("list").executes(commandContext3 -> {
            MixinMessageIndicatorHelper.do344(Text.of("%d chat filter(s)".formatted(Integer.valueOf(BaritoneHelper_3.chatFilterSearchHelper4.get2681()))), MixinMessageIndicatorHelper.getMessageSignatureData337(-3569855));
            int i = -3469852;
            Iterator<String> it = BaritoneHelper_3.chatFilterSearchHelper4.getList2682().iterator();
            while (it.hasNext()) {
                i++;
                MixinMessageIndicatorHelper.do344(Text.of(it.next()), MixinMessageIndicatorHelper.getMessageSignatureData337(i));
            }
            return 1;
        }));
    }
}
