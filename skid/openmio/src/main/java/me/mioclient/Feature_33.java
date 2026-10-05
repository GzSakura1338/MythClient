package me.mioclient;

import com.mojang.brigadier.exceptions.CommandSyntaxException;
import java.util.List;
import net.minecraft.command.CommandSource;
import net.minecraft.command.DataCommandObject;
import net.minecraft.command.EntityDataObject;
import net.minecraft.command.argument.NbtPathArgumentType;
import net.minecraft.nbt.NbtElement;
import net.minecraft.nbt.NbtHelper;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_33.class */
public class Feature_33 extends Feature {
    public Feature_33() {
        super("nbt");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.executes(commandContext -> {
            DataCommandObject entityDataObject = new EntityDataObject(minecraftClient.player);
            NbtPathArgumentType.NbtPath parse = NbtPathArgumentType.NbtPath.parse("SelectedItem");
            MutableText append = Text.empty().append("Nbt: ");
            try {
                List list = parse.get(entityDataObject.getNbt());
                if (!list.isEmpty()) {
                    append.append(" ").append(NbtHelper.toPrettyPrintedText((NbtElement) list.getFirst()));
                }
            } catch (CommandSyntaxException e) {
                append.append("{}");
            }
            MixinMessageIndicatorHelper.do344((Text) append, MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        });
    }
}
