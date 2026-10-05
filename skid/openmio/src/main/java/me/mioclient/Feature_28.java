package me.mioclient;

import java.io.File;
import java.util.ArrayList;
import me.mioclient.module.client.UI;
import net.minecraft.command.CommandSource;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_28.class */
public final class Feature_28 extends Feature {
    public Feature_28() {
        super("help");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        if (!new File("mio-fabric").exists()) {
            BaritoneHelper_3.tooltipsSearchHelper4_2.do164(() -> {
                MixinMessageIndicatorHelper.do344(Text.literal("Welcome to Mio!"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1001));
                MixinMessageIndicatorHelper.do344(Text.literal("The UI bind is ").append(getMutableText341(UI.uI.getKeybind().getString773())), MixinMessageIndicatorHelper.getMessageSignatureData337(-1002));
                MixinMessageIndicatorHelper.do344(Text.literal("The command prefix is ").append(getMutableText341(ChatFilterSearchHelper4_2.getString2982())), MixinMessageIndicatorHelper.getMessageSignatureData337(-1003));
            }, 0);
        }
        literalArgumentBuilder.executes(commandContext -> {
            ArrayList arrayList = new ArrayList();
            BaritoneHelper_3.chatFilterSearchHelper4_2.getRegistry().forEach(feature -> {
                arrayList.add(feature.getName());
            });
            Text empty = Text.empty();
            ((MutableText) empty).append("Commands (%d): ".formatted(Integer.valueOf(arrayList.size())));
            ((MutableText) empty).append(Text.of(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.join(", ", arrayList)).getString2921("\u0001.")));
            MixinMessageIndicatorHelper.do344(empty, MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        });
    }
}
