package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.io.FileNotFoundException;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.feature.Enum;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.text.Texts;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_10.class */
public final class Feature_10 extends Feature {
    public final NameTagsHelperMode nameTagsHelperMode;

    public Feature_10(NameTagsHelperMode nameTagsHelperMode) {
        super(nameTagsHelperMode.name().toLowerCase());
        this.nameTagsHelperMode = nameTagsHelperMode;
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("add").then(Feature.argument("name", new ExamplesArgumentType_3()).executes(commandContext -> {
            String str = (String) commandContext.getArgument("name", String.class);
            if (BaritoneHelper_3.searchHelper4_14.getNameTagsHelperMode525(str) == this.nameTagsHelperMode) {
                MixinMessageIndicatorHelper.do344(getMutableText341(str).append(getString1345(" is already in your %s list")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
                return 1;
            }
            if (this.nameTagsHelperMode == NameTagsHelperMode.ENEMY) {
                BaritoneHelper_3.searchHelper4_14.do524(str);
            } else {
                BaritoneHelper_3.searchHelper4_14.do523(str);
            }
            MixinMessageIndicatorHelper.do344(getMutableText341(str).append(getString1345(" has been added to your %s list")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })));
        literalArgumentBuilder.then(Feature.getRequiredArgumentBuilder411("remove", "delete", "del").then(Feature.argument("name", StringArgumentType.word()).suggests(this::getCompletableFuture1344).executes(commandContext2 -> {
            String str = (String) commandContext2.getArgument("name", String.class);
            if (BaritoneHelper_3.searchHelper4_14.getNameTagsHelperMode525(str) != this.nameTagsHelperMode) {
                MixinMessageIndicatorHelper.do344(getMutableText341(str).append(getString1345(" is not your %s")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
                return 1;
            }
            BaritoneHelper_3.searchHelper4_14.is527(str);
            MixinMessageIndicatorHelper.do344(getMutableText341(str).append(getString1345(" has been removed from your %s list")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })));
        literalArgumentBuilder.then(Feature.literal("list").executes(commandContext3 -> {
            List list = BaritoneHelper_3.searchHelper4_14.getRegistry().stream().filter(mixinPlayerEntityData -> {
                return mixinPlayerEntityData.getNameTagsHelperMode631() == this.nameTagsHelperMode;
            }).map(mixinPlayerEntityData2 -> {
                return Text.of(mixinPlayerEntityData2.getName());
            }).toList();
            MixinMessageIndicatorHelper.do344(Text.literal(getString1345("%S list: ")).append(Texts.join(list, Text.of(", "))), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }));
        literalArgumentBuilder.then(Feature.literal("sync").then(Feature.argument("importer", new Enum(Mode.class, "importer")).executes(commandContext4 -> {
            Mode mode = (Mode) commandContext4.getArgument("importer", Mode.class);
            try {
                int i = 0;
                for (String str : mode.getList210()) {
                    if (!BaritoneHelper_3.searchHelper4_14.is519(str)) {
                        BaritoneHelper_3.searchHelper4_14.do523(str);
                        i++;
                    }
                }
                MixinMessageIndicatorHelper.do344(Text.literal(String.format("Added %d new friends from %s", Integer.valueOf(i), mode.getString209())), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
                return 1;
            } catch (Exception e) {
                if (e instanceof FileNotFoundException) {
                    MixinMessageIndicatorHelper.do345(Text.literal(String.format("Couldn't find %s friends file", mode.getString209())).styled(style -> {
                        return style.withColor(Formatting.RED);
                    }), MixinMessageIndicatorHelper.getMessageSignatureData337(-1), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode3);
                    return 1;
                }
                e.printStackTrace();
                MixinMessageIndicatorHelper.do345(Text.literal(String.format("Couldn't import friends from %s due to an unknown error, check your log file", mode.getString209())).styled(style2 -> {
                    return style2.withColor(Formatting.RED);
                }), MixinMessageIndicatorHelper.getMessageSignatureData337(-1), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode3);
                return 1;
            }
        })));
    }

    public CompletableFuture<Suggestions> getCompletableFuture1344(CommandContext<CommandSource> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(BaritoneHelper_3.searchHelper4_14.getRegistry().stream().filter(mixinPlayerEntityData -> {
            return mixinPlayerEntityData.getNameTagsHelperMode631() == this.nameTagsHelperMode;
        }).map((v0) -> {
            return v0.getName();
        }), suggestionsBuilder);
    }

    public String getString1345(String str) {
        return str.replace("%s", this.nameTagsHelperMode.getString1987()).replace("%S", FontsSearchHelper4.getString1684(this.nameTagsHelperMode.getString1987()));
    }
}
