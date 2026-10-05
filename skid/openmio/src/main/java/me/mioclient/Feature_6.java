package me.mioclient;

import com.mojang.brigadier.arguments.IntegerArgumentType;
import com.mojang.brigadier.arguments.StringArgumentType;
import com.mojang.brigadier.builder.ArgumentBuilder;
import com.mojang.brigadier.builder.RequiredArgumentBuilder;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.context.StringRange;
import com.mojang.brigadier.suggestion.Suggestion;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;
import me.mioclient.api.Keybind;
import me.mioclient.event.KeyEvent;
import me.mioclient.event.Listen;
import me.mioclient.feature.Enum;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_6.class */
public final class Feature_6 extends Feature {
    public boolean flag;
    public String string;
    public Mode_4 mode_4;

    public Feature_6() {
        super("macro");
        baritoneHelper.do1796(this);
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(getLiteralArgumentBuilder781("delete", requiredArgumentBuilder -> {
            requiredArgumentBuilder.executes(commandContext -> {
                KeybindFeature keybindFeature = (KeybindFeature) commandContext.getArgument("macro", KeybindFeature.class);
                BaritoneHelper_3.searchHelper4_12.unregister(keybindFeature);
                MixinMessageIndicatorHelper.do344(Text.literal(FontsSearchHelper4.getString1684(keybindFeature.getName())).append(" has been deleted"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
                return 1;
            });
        })).then(getLiteralArgumentBuilder781("commands", this::do780)).then(Feature.literal("new").then(Feature.argument("name", StringArgumentType.word()).then(Feature.argument("type", new Enum(Mode_4.class, "Macro")).then(Feature.argument("key", new ArgumentType_4()).then(getRequiredArgumentBuilder783().executes(commandContext -> {
            String str = (String) commandContext.getArgument("name", String.class);
            String str2 = (String) commandContext.getArgument("command", String.class);
            KeybindFeature keybindFeature832 = ((Mode_4) commandContext.getArgument("type", Mode_4.class)).getKeybindFeature832(str, (Keybind) commandContext.getArgument("key", Keybind.class));
            keybindFeature832.getList2059().add(str2);
            BaritoneHelper_3.searchHelper4_12.register(keybindFeature832);
            MixinMessageIndicatorHelper.do344(Text.literal("Macro ").append(keybindFeature832.getText1879()).append(" has been created"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })).executes(commandContext2 -> {
            String str = (String) commandContext2.getArgument("name", String.class);
            KeybindFeature keybindFeature832 = ((Mode_4) commandContext2.getArgument("type", Mode_4.class)).getKeybindFeature832(str, (Keybind) commandContext2.getArgument("key", Keybind.class));
            BaritoneHelper_3.searchHelper4_12.register(keybindFeature832);
            MixinMessageIndicatorHelper.do344(Text.literal("Macro ").append(keybindFeature832.getText1879()).append(" has been created"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })).executes(commandContext3 -> {
            this.string = (String) commandContext3.getArgument("name", String.class);
            this.mode_4 = (Mode_4) commandContext3.getArgument("type", Mode_4.class);
            this.flag = true;
            MixinMessageIndicatorHelper.do344(Text.literal("Press a key"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })))).then(Feature.literal("list").executes(commandContext4 -> {
            int i = 0;
            MixinMessageIndicatorHelper.do344(Text.literal("Macro list:"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            for (KeybindFeature keybindFeature : BaritoneHelper_3.searchHelper4_12.getRegistry()) {
                MixinMessageIndicatorHelper.do344(Text.literal("%s (%s)".formatted(keybindFeature.getName(), keybindFeature.getKeybind().getString773())), MixinMessageIndicatorHelper.getMessageSignatureData337(i));
                i++;
            }
            return 0;
        }));
    }

    public void do780(RequiredArgumentBuilder<CommandSource, KeybindFeature> requiredArgumentBuilder) {
        requiredArgumentBuilder.then(Feature.literal("add").then(getRequiredArgumentBuilder783().executes(commandContext -> {
            KeybindFeature keybindFeature = (KeybindFeature) commandContext.getArgument("macro", KeybindFeature.class);
            String str = (String) commandContext.getArgument("command", String.class);
            if (keybindFeature.getMode_42058() != Mode_4.HOLD || keybindFeature.getList2059().size() < 2) {
                keybindFeature.getList2059().add(str);
                return 1;
            }
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("\u0001You've reached the command limit of Hold Macro")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }))).then(Feature.literal("remove").then(Feature.argument("index", IntegerArgumentType.integer(1)).executes(commandContext2 -> {
            KeybindFeature keybindFeature = (KeybindFeature) commandContext2.getArgument("macro", KeybindFeature.class);
            int intValue = ((Integer) commandContext2.getArgument("index", Integer.class)).intValue() - 1;
            if (intValue < keybindFeature.getList2059().size() && intValue >= 0 && !keybindFeature.getList2059().isEmpty()) {
                keybindFeature.getList2059().remove(intValue);
                return 1;
            }
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(getString782(keybindFeature)).getArgumentTypeHelper2919(String.valueOf(Formatting.WHITE)).getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("\u0001Invalid index\u0001\n\u0001")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }))).then(Feature.literal("clear").executes(commandContext3 -> {
            ((KeybindFeature) commandContext3.getArgument("macro", KeybindFeature.class)).getList2059().clear();
            return 1;
        })).then(Feature.literal("edit").then(Feature.argument("index", IntegerArgumentType.integer(1)).then(getRequiredArgumentBuilder783().executes(commandContext4 -> {
            KeybindFeature keybindFeature = (KeybindFeature) commandContext4.getArgument("macro", KeybindFeature.class);
            String str = (String) commandContext4.getArgument("command", String.class);
            int intValue = ((Integer) commandContext4.getArgument("index", Integer.class)).intValue() - 1;
            if (intValue <= keybindFeature.getList2059().size()) {
                keybindFeature.getList2059().set(intValue, str);
                return 1;
            }
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(getString782(keybindFeature)).getArgumentTypeHelper2919(String.valueOf(Formatting.WHITE)).getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("\u0001Invalid index\u0001\n\u0001")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })))).executes(commandContext5 -> {
            MixinMessageIndicatorHelper.do344(Text.literal(getString782((KeybindFeature) commandContext5.getArgument("macro", KeybindFeature.class))), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        });
    }

    public com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> getLiteralArgumentBuilder781(String str, Consumer<RequiredArgumentBuilder<CommandSource, KeybindFeature>> consumer) {
        RequiredArgumentBuilder<CommandSource, KeybindFeature> argument = Feature.argument("macro", new ArgumentType_7());
        consumer.accept(argument);
        return Feature.literal(str).then(argument);
    }

    public String getString782(KeybindFeature keybindFeature) {
        StringBuffer stringBuffer = new StringBuffer(new ArgumentTypeHelper().getArgumentTypeHelper2919(keybindFeature.getName()).getString2921("\u0001's command list: \n"));
        for (int i = 0; i < keybindFeature.getList2059().size(); i++) {
            stringBuffer.append(i + 1);
            stringBuffer.append(". ");
            stringBuffer.append(keybindFeature.getList2059().get(i));
            if (i != keybindFeature.getList2059().size() - 1) {
                stringBuffer.append("\n");
            }
        }
        return stringBuffer.toString();
    }

    public RequiredArgumentBuilder<CommandSource, String> getRequiredArgumentBuilder783() {
        return Feature.argument("command", StringArgumentType.greedyString()).suggests(this::getCompletableFuture784);
    }

    public CompletableFuture<Suggestions> getCompletableFuture784(CommandContext<CommandSource> commandContext, SuggestionsBuilder suggestionsBuilder) {
        try {
            int start = suggestionsBuilder.getStart();
            int i = 0;
            StringBuilder sb = new StringBuilder();
            for (char c : suggestionsBuilder.getRemaining().toCharArray()) {
                i++;
                if (c == ChatFilterSearchHelper4_2.get2984() || (i == 1 && c == ' ')) {
                    start += i;
                    i = 0;
                    sb.setLength(0);
                } else {
                    sb.append(c);
                }
            }
            int i2 = start;
            return ChatFilterSearchHelper4_2.commandDispatcher.getCompletionSuggestions(ChatFilterSearchHelper4_2.commandDispatcher.parse(sb.toString(), (CommandSource) null), i).thenApply(suggestions -> {
                StringRange range = suggestions.getRange();
                return new Suggestions(new StringRange(range.getStart() + i2, range.getEnd() + i2), suggestions.getList().stream().map(suggestion -> {
                    return new Suggestion(new StringRange(suggestion.getRange().getStart() + i2, suggestion.getRange().getEnd() + i2), suggestion.getText(), suggestion.getTooltip());
                }).toList());
            });
        } catch (Exception e) {
            return suggestionsBuilder.buildFuture();
        }
    }

    @Listen
    public void onKey(KeyEvent keyEvent) {
        if (!this.flag || this.mode_4 == null || this.string == null) {
            return;
        }
        KeybindFeature keybindFeature832 = this.mode_4.getKeybindFeature832(this.string, new Keybind(keyEvent.get2587(), Keybind.KeybindMode.TOGGLE, keyEvent.is2588()));
        BaritoneHelper_3.searchHelper4_12.register(keybindFeature832);
        MixinMessageIndicatorHelper.do344(Text.literal("Macro ").append(keybindFeature832.getText1879()).append(" has been created"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
        this.flag = false;
    }
}
