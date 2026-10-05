package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.mojang.brigadier.arguments.StringArgumentType;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Keybind;
import me.mioclient.feature.Enum;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;
import net.minecraft.text.Texts;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_13.class */
public final class Feature_13 extends Feature {
    public Feature_13() {
        super("preset");
        do414("config", "cfg");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("category", new ArgumentType_6()).then(Feature.literal("save").then(Feature.argument("preset", StringArgumentType.string()).executes(commandContext -> {
            String str = (String) commandContext.getArgument("preset", String.class);
            try {
                ArgumentType_6.getManager(commandContext, "category").do39(str);
            } catch (Exception e) {
                e.printStackTrace();
            }
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("Preset \u0001 has been saved.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }))).then(Feature.literal("delete").then(Feature.argument("preset", new ArgumentType_9()).executes(commandContext2 -> {
            PresetSearchHelper4 preset = ArgumentType_9.getPreset(commandContext2, "category", "preset");
            ArgumentType_6.getManager(commandContext2, "category").is36(preset.getString333());
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(preset.getString333()).getString2921("Preset \u0001 has been deleted.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }))).then(Feature.literal("rename").then(Feature.argument("preset", new ArgumentType_9()).then(Feature.argument("name", StringArgumentType.string()).executes(commandContext3 -> {
            PresetSearchHelper4 preset = ArgumentType_9.getPreset(commandContext3, "category", "preset");
            ArgumentType_6.getManager(commandContext3, "category").is38(preset.getString333(), (String) commandContext3.getArgument("name", String.class));
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919((String) commandContext3.getArgument("name", String.class)).getArgumentTypeHelper2919(preset.getString333()).getString2921("The name of \u0001 has been set to \u0001.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })))).then(Feature.literal("load").then(Feature.argument("preset", new ArgumentType_9()).executes(commandContext4 -> {
            BaritoneHelper_3.presetHelper.getPresetHelper_374().do41();
            PresetHelperMode presetHelperMode = (PresetHelperMode) commandContext4.getArgument("category", PresetHelperMode.class);
            PresetSearchHelper4 preset = ArgumentType_9.getPreset(commandContext4, "category", "preset");
            PresetHelperSearchHelper4_2 manager = ArgumentType_6.getManager(commandContext4, "category");
            manager.do34();
            manager.is35(preset.getString333());
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(preset.getString333()).getString2921("Preset \u0001 has been loaded.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            if (presetHelperMode != PresetHelperMode.ALL) {
                return 1;
            }
            do413(this::do1575);
            return 1;
        }))).then(Feature.literal("specific").then(Feature.argument("preset", new ArgumentType_9()).then(Feature.argument("module_category", new Enum(Category.class, "Category")).executes(commandContext5 -> {
            BaritoneHelper_3.presetHelper.getPresetHelper_374().do41();
            PresetSearchHelper4 preset = ArgumentType_9.getPreset(commandContext5, "category", "preset");
            PresetHelperSearchHelper4_2 manager = ArgumentType_6.getManager(commandContext5, "category");
            Category category = (Category) commandContext5.getArgument("module_category", Category.class);
            manager.do34();
            do1573(preset, module -> {
                return module.getCategory() == category;
            });
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(category.getName()).getArgumentTypeHelper2919(preset.getString333()).getString2921("Preset \u0001 for \u0001 has been loaded.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })).then(Feature.argument("modules", new ArgumentType_5(new ArgumentType_8())).executes(commandContext6 -> {
            BaritoneHelper_3.presetHelper.getPresetHelper_374().do41();
            PresetSearchHelper4 preset = ArgumentType_9.getPreset(commandContext6, "category", "preset");
            PresetHelperSearchHelper4_2 manager = ArgumentType_6.getManager(commandContext6, "category");
            Set set = (Set) ((List) commandContext6.getArgument("modules", List.class)).stream().map((v0) -> {
                return v0.getClass();
            }).collect(Collectors.toSet());
            manager.do34();
            java.util.function.Predicate<Module> predicate = module -> {
                return set.contains(module.getClass());
            };
            do1573(preset, predicate);
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(getString1574(predicate)).getArgumentTypeHelper2919(preset.getString333()).getString2921("Preset \u0001 for \u0001 has been loaded.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        })))).then(Feature.literal("list").executes(commandContext7 -> {
            PresetHelperSearchHelper4_2 manager = ArgumentType_6.getManager(commandContext7, "category");
            MixinMessageIndicatorHelper.do344(Text.literal("Preset list: ").append(Texts.join(manager.getRegistry(), presetSearchHelper4 -> {
                return Text.literal(presetSearchHelper4.getString333());
            })).append("."), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }))).then(Feature.literal("export").then(Feature.argument("module", new ArgumentType_8()).executes(commandContext8 -> {
            Module module = (Module) commandContext8.getArgument("module", Module.class);
            JsonObject asJsonObject = module.toJson().getAsJsonObject();
            asJsonObject.remove("bind");
            asJsonObject.remove("toggled");
            asJsonObject.remove("key");
            minecraftClient.keyboard.setClipboard(Base64.getEncoder().encodeToString(asJsonObject.toString().getBytes(StandardCharsets.UTF_8)));
            MixinMessageIndicatorHelper.do344(Text.literal("Successfully copied ").append(Text.literal(module.getName()).formatted(Formatting.GRAY)).append(" config to your clipboard"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }))).then(Feature.literal("import").then(Feature.argument("module", new ArgumentType_8()).executes(commandContext9 -> {
            Module module = (Module) commandContext9.getArgument("module", Module.class);
            try {
                module.fromJson(JsonParser.parseString(new String(Base64.getDecoder().decode(minecraftClient.keyboard.getClipboard().trim().replace("\n", "")))));
                MixinMessageIndicatorHelper.do344(Text.literal("Successfully loaded ").append(Text.literal(module.getName()).formatted(Formatting.GRAY)).append(" config from your clipboard"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
                return 1;
            } catch (Exception e) {
                MutableText append = Text.literal("Failed to load ").append(Text.literal(module.getName()).formatted(Formatting.GRAY)).append(" config");
                MixinMessageIndicatorHelper.do345((Text) (e.getStackTrace()[0].toString().contains("Base64") ? append.append(" due to invalid input") : append.append(" due to an unknown reason")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
                e.printStackTrace();
                return 1;
            }
        })));
        literalArgumentBuilder.then(Feature.literal("restore").executes(commandContext10 -> {
            JsonElement jsonElement1434 = BaritoneHelper_3.presetHelper.getPresetHelper_374().getJsonElement1434();
            if (jsonElement1434 == null) {
                return 1;
            }
            PresetHelperMode.ALL.fromJson(jsonElement1434);
            MixinMessageIndicatorHelper.do344(Text.literal("Restored previous preset."), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }));
        literalArgumentBuilder.executes(commandContext11 -> {
            do413(() -> {
                minecraftClient.setScreen(new PresetFontsSearchHelper42());
            });
            return 1;
        });
    }

    public void do1573(PresetSearchHelper4 presetSearchHelper4, java.util.function.Predicate<Module> predicate) {
        for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
            if (!(module instanceof ModuleList) && predicate.test(module)) {
                try {
                    module.fromJson(presetSearchHelper4.getJsonObject2741().getAsJsonObject().get(module.getConfigName()));
                } catch (Exception e) {
                }
            }
        }
    }

    public String getString1574(java.util.function.Predicate<Module> predicate) {
        return (String) BaritoneHelper_3.keyPearlSearchHelper4.getRegistry().stream().filter(predicate).map((v0) -> {
            return v0.getName();
        }).collect(Collectors.joining(", "));
    }

    public void do1575() {
        MutableText empty = Text.empty();
        empty.append("Preset binds: ");
        for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
            Keybind keybind = module.getKeybind();
            if (!keybind.is1944()) {
                empty.append("\n");
                empty.append(module.getName());
                empty.append(" - ");
                empty.append(keybind.getString773());
                empty.append(" ");
                empty.append(MixinMessageIndicatorHelper.getText349(keybind.getKeybindMode1946().getName()));
            }
        }
        MixinMessageIndicatorHelper.do343((Text) empty, -2);
    }
}
