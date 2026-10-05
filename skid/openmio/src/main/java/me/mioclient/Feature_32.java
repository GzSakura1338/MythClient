package me.mioclient;

import com.mojang.brigadier.arguments.BoolArgumentType;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.awt.Color;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.SwitchBootstraps;
import java.util.Collection;
import java.util.Iterator;
import java.util.Objects;
import java.util.concurrent.CompletableFuture;
import java.util.stream.Stream;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;
import net.minecraft.text.ClickEvent;
import net.minecraft.text.HoverEvent;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;
import net.minecraft.text.Texts;
import net.minecraft.util.Formatting;
import org.jetbrains.annotations.NotNull;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_32.class */
public final class Feature_32 extends Feature {
    public final Module module;

    public Feature_32(Module module) {
        super(module.getName());
        this.module = module;
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("state", BoolArgumentType.bool()).suggests((commandContext, suggestionsBuilder) -> {
            return suggestionsBuilder.buildFuture();
        }).executes(commandContext2 -> {
            this.module.do495(((Boolean) commandContext2.getArgument("state", Boolean.class)).booleanValue());
            return 1;
        })).then(Feature.argument("setting", new ArgumentType(this.module)).then(Feature.getRequiredArgumentBuilder411("clear").suggests(this::getCompletableFuture2531).executes(commandContext3 -> {
            Setting<?> option = ArgumentType.getOption(commandContext3, this.module, "setting");
            if (!(option instanceof ObjectSetting)) {
                return 1;
            }
            ((ObjectSetting<?>) option).getValue().clear();
            MixinMessageIndicatorHelper.do345(Text.literal(this.module.getName()).append(Text.literal(" ").append(option.getName()).styled(style -> {
                return style.withFormatting(Formatting.GRAY);
            })).append(": Cleared"), MixinMessageIndicatorHelper.getMessageSignatureData339(this.module), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode3);
            return 1;
        })).then(Feature.argument("value", new ArgumentType_3(this.module, "setting")).executes(commandContext4 -> {
            Setting<?> option = ArgumentType.getOption(commandContext4, this.module, "setting");
            String str = (String) commandContext4.getArgument("value", String.class);
            try {
                Formatting formatting = null;
                if (option instanceof EntityListObjectSetting) {
                    EntityListObjectSetting entityListObjectSetting = (EntityListObjectSetting) option;
                    Collection<String> collection53 = Helper.getHelper56(entityListObjectSetting.getRegistry2387()).getCollection53(str);
                    if (!collection53.isEmpty()) {
                        Stream<String> stream = collection53.stream();
                        Objects.requireNonNull(entityListObjectSetting);
                        if (stream.anyMatch(entityListObjectSetting::is3133)) {
                            Objects.requireNonNull(entityListObjectSetting);
                            collection53.forEach(entityListObjectSetting::do3131);
                            formatting = Formatting.RED;
                        } else {
                            Objects.requireNonNull(entityListObjectSetting);
                            collection53.forEach(entityListObjectSetting::do3129);
                            formatting = Formatting.GREEN;
                        }
                    }
                }
                if (formatting == null) {
                    option.do134(str);
                }
                MutableText text2532 = (MutableText)(getText2532(option));
                if (option instanceof ObjectSetting) {
                    ObjectSetting objectSetting = (ObjectSetting) option;
                    if (formatting == null) {
                        formatting = objectSetting.is3133(str) ? Formatting.GREEN : Formatting.RED;
                    }
                    Formatting formatting2 = formatting;
                    text2532 = Text.literal(str).styled(style -> {
                        return style.withFormatting(formatting2);
                    });
                }
                MixinMessageIndicatorHelper.do345(Text.literal(this.module.getName()).append(Text.literal(" ").append(option.getName()).styled(style2 -> {
                    return style2.withFormatting(Formatting.GRAY);
                })).append(": ").append((Text) text2532), MixinMessageIndicatorHelper.getMessageSignatureData339(this.module), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode3);
                return 1;
            } catch (Throwable th) {
                MixinMessageIndicatorHelper.do345(Text.literal("Invalid value: ").append(str), MixinMessageIndicatorHelper.getMessageSignatureData339(this.module), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode);
                return 1;
            }
        })).then(Feature.literal("reset").executes(commandContext5 -> {
            ArgumentType.getOption(commandContext5, this.module, "setting").reset();
            return 1;
        })).executes(commandContext6 -> {
            Setting<?> option = ArgumentType.getOption(commandContext6, this.module, "setting");
            MixinMessageIndicatorHelper.do344(Text.literal(this.module.getName()).append(Text.literal(" ").append(option.getName()).styled(style -> {
                return style.withFormatting(Formatting.GRAY);
            })).append(": ").append(getText2532(option)), MixinMessageIndicatorHelper.getMessageSignatureData339(this.module));
            return 1;
        })).then(Feature.literal("Enabled").then(Feature.argument("state", BoolArgumentType.bool()).executes(commandContext7 -> {
            this.module.do495(((Boolean) commandContext7.getArgument("state", Boolean.class)).booleanValue());
            return 1;
        })).executes(commandContext8 -> {
            MixinMessageIndicatorHelper.do344(Text.literal("Module ").append(Text.literal(this.module.getName()).styled(style -> {
                return style.withFormatting(Formatting.GRAY);
            })).append(" is ").append(Text.literal(this.module.isToggled() ? "enabled" : "disabled").styled(style2 -> {
                return this.module.isToggled() ? style2.withFormatting(Formatting.GREEN) : style2.withFormatting(Formatting.RED);
            })), MixinMessageIndicatorHelper.getMessageSignatureData339(this.module));
            return 1;
        })).executes(commandContext9 -> {
            MutableText empty = Text.empty();
            empty.append(getMutableText2529()).append(" [%s - %s]".formatted(this.module.getKeybind().getString773(), this.module.getKeybind().getKeybindMode1946().getName()));
            for (Setting<?> setting : this.module.getRegistry()) {
                MutableText literal = Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(setting.getName()).getString2921("\n\u0001: "));
                Objects.requireNonNull(setting);
                int typeSwitchIndex = setting instanceof EnumSetting ? 0
                : setting instanceof ObjectSetting ? 1
                : setting instanceof ColorSetting ? 2
                : -1;
        switch (typeSwitchIndex) {
                    case 0:
                        literal.append(Text.literal(FontsSearchHelper4.getString1684(((Enum) ((EnumSetting) setting).getValue()).name())).formatted(Formatting.GRAY));
                        break;
                    case 1:
                        literal.append(Text.literal(String.join(", ", ((ObjectSetting) setting).getSet3126())).formatted(Formatting.GRAY));
                        break;
                    case 2:
                        ColorSetting colorSetting = (ColorSetting) setting;
                        literal.append(Text.literal(getString2530(colorSetting)).styled(style -> {
                            return style.withColor(colorSetting.getValue().hashCode());
                        }));
                        break;
                    default:
                        literal.append(Text.literal(setting.getValue().toString()).formatted(Formatting.GRAY));
                        break;
                }
                String string2921 = new ArgumentTypeHelper().getArgumentTypeHelper2919(setting.getName()).getArgumentTypeHelper2919(this.module.getName().toLowerCase()).getArgumentTypeHelper2919(ChatFilterSearchHelper4_2.getString2982()).getString2921("\u0001\u0001 \u0001 ");
                literal.styled(style2 -> {
                    return style2.withClickEvent(new ClickEvent(ClickEvent.Action.SUGGEST_COMMAND, string2921));
                });
                empty.append((Text) literal);
            }
            MixinMessageIndicatorHelper.do344((Text) empty, MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        });
        literalArgumentBuilder.then(Feature.literal("reset").executes(commandContext10 -> {
            Iterator<Setting<?>> it = this.module.getRegistry().iterator();
            while (it.hasNext()) {
                it.next().reset();
            }
            return 1;
        }));
    }

    @NotNull
    public MutableText getMutableText2529() {
        MutableText literal = Text.literal(this.module.getName());
        literal.styled(style -> {
            return MixinMessageIndicatorHelper.getStyle340(style, () -> {
                return this.module.isToggled() ? Formatting.GREEN.getColorValue() : Formatting.RED.getColorValue();
            });
        });
        literal.styled(style2 -> {
            return style2.withHoverEvent(new HoverEvent(HoverEvent.Action.SHOW_TEXT, Text.literal(this.module.getDescription().split("\n")[0])));
        });
        literal.styled(style3 -> {
            return style3.withClickEvent(new ClickEvent(ClickEvent.Action.RUN_COMMAND, new ArgumentTypeHelper().getArgumentTypeHelper2919(this.module.getName()).getArgumentTypeHelper2919(ChatFilterSearchHelper4_2.getString2982()).getString2921("\u0001toggle \u0001")));
        });
        return literal;
    }

    public String getString2530(ColorSetting colorSetting) {
        Color value = colorSetting.getValue();
        return colorSetting.flag3 ? "rgb(%d, %d, %d)".formatted(Integer.valueOf(value.getRed()), Integer.valueOf(value.getGreen()), Integer.valueOf(value.getBlue())) : "rgba(%d, %d, %d, %d)".formatted(Integer.valueOf(value.getRed()), Integer.valueOf(value.getGreen()), Integer.valueOf(value.getBlue()), Integer.valueOf(value.getAlpha()));
    }

    public CompletableFuture<Suggestions> getCompletableFuture2531(CommandContext<?> commandContext, SuggestionsBuilder suggestionsBuilder) {
        try {
            if (ArgumentType.getOption(commandContext, this.module, "setting") instanceof ObjectSetting) {
                return CommandSource.suggestMatching(new String[]{"clear"}, suggestionsBuilder);
            }
        } catch (Exception e) {
        }
        return Suggestions.empty();
    }

    public Text getText2532(Setting<?> setting) {
        MutableText literal = Text.literal(setting.getValue().toString());
        if (setting instanceof ColorSetting) {
            literal = Text.literal(MixinMessageIndicatorHelper_2.getString826(((ColorSetting) setting).getValue(), true)).styled(style -> {
                return style.withColor(setting.getValue().hashCode());
            });
        } else if (setting instanceof ObjectSetting) {
            literal = Texts.join(((ObjectSetting) setting).getSet3126(), Text.literal(", ").styled(style2 -> {
                return style2.withFormatting(Formatting.GRAY);
            }), Text::literal);
        } else if (setting instanceof SearchIdentifierSetting) {
            literal = Text.literal(((SearchIdentifierSetting) setting).getValue().getName());
        }
        return literal;
    }
}
