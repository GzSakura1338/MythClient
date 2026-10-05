package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.concurrent.CompletableFuture;
import me.mioclient.feature.Enum;
import me.mioclient.module.render.Waypoints;
import net.minecraft.client.network.ServerInfo;
import net.minecraft.command.CommandSource;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;
import net.minecraft.text.Texts;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_25.class */
public final class Feature_25 extends Feature {
    public Feature_25() {
        super("waypoints");
        do414("wp");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("add").then(Feature.argument("name", StringArgumentType.string()).then(Feature.argument("pos", new ExamplesArgumentType()).executes(commandContext -> {
            WaypointsEnumSettingHelper waypointsEnumSettingHelper = new WaypointsEnumSettingHelper((String) commandContext.getArgument("name", String.class), (Vec3d) commandContext.getArgument("pos", Vec3d.class), SearchHelper4_7.getStashFinderMode2438().getString2175().toLowerCase(), getString2125());
            BaritoneHelper_3.waypointsSearchHelper4.register(waypointsEnumSettingHelper);
            MixinMessageIndicatorHelper.do343(Text.literal("Waypoint ").append(waypointsEnumSettingHelper.getText1879()).append(" has been created"), -1);
            return 1;
        }).then(Feature.argument("dimension", new Enum(StashFinderMode.class, "dimension")).executes(commandContext2 -> {
            WaypointsEnumSettingHelper waypointsEnumSettingHelper = new WaypointsEnumSettingHelper((String) commandContext2.getArgument("name", String.class), (Vec3d) commandContext2.getArgument("pos", Vec3d.class), ((StashFinderMode) commandContext2.getArgument("dimension", StashFinderMode.class)).getString2175().toLowerCase(), getString2125());
            BaritoneHelper_3.waypointsSearchHelper4.register(waypointsEnumSettingHelper);
            MixinMessageIndicatorHelper.do343(Text.literal("Waypoint ").append(waypointsEnumSettingHelper.getText1879()).append(" has been created"), -1);
            return 1;
        }))).executes(commandContext3 -> {
            WaypointsEnumSettingHelper waypointsEnumSettingHelper = new WaypointsEnumSettingHelper((String) commandContext3.getArgument("name", String.class), minecraftClient.player.getPos(), SearchHelper4_7.getStashFinderMode2438().getString2175().toLowerCase(), getString2125());
            BaritoneHelper_3.waypointsSearchHelper4.register(waypointsEnumSettingHelper);
            MixinMessageIndicatorHelper.do343(Text.literal("Waypoint ").append(waypointsEnumSettingHelper.getText1879()).append(" has been created"), -1);
            return 1;
        }))).then(Feature.getRequiredArgumentBuilder411("remove", "delete", "del").then(Feature.argument("server", StringArgumentType.string()).suggests(Feature_25::getCompletableFuture2121).then(Feature.argument("name", StringArgumentType.string()).suggests(Feature_25::getCompletableFuture2123).executes(commandContext4 -> {
            String str = (String) commandContext4.getArgument("name", String.class);
            String str2 = (String) commandContext4.getArgument("server", String.class);
            BaritoneHelper_3.waypointsSearchHelper4.getRegistry().removeIf(waypointsEnumSettingHelper -> {
                return str2.equalsIgnoreCase(waypointsEnumSettingHelper.getString518()) && str.equalsIgnoreCase(waypointsEnumSettingHelper.getName());
            });
            MixinMessageIndicatorHelper.do343(Text.literal("Waypoint ").append(str).append(" has been removed"), -1);
            return 1;
        })))).then(Feature.literal("rename").then(Feature.argument("server", StringArgumentType.string()).suggests(Feature_25::getCompletableFuture2121).then(Feature.argument("name", StringArgumentType.string()).suggests(Feature_25::getCompletableFuture2123).then(Feature.argument("target", StringArgumentType.string()).executes(commandContext5 -> {
            String str = (String) commandContext5.getArgument("name", String.class);
            String str2 = (String) commandContext5.getArgument("server", String.class);
            String str3 = (String) commandContext5.getArgument("target", String.class);
            java.util.function.Predicate<WaypointsEnumSettingHelper> predicate = waypointsEnumSettingHelper -> {
                return str2.equalsIgnoreCase(waypointsEnumSettingHelper.getString518()) && str.equalsIgnoreCase(waypointsEnumSettingHelper.getName());
            };
            WaypointsEnumSettingHelper orElse = BaritoneHelper_3.waypointsSearchHelper4.getOptional2404(predicate).orElse(null);
            if (orElse == null) {
                MixinMessageIndicatorHelper.do343(Text.of("Waypoint not found"), -1);
                return 1;
            }
            BaritoneHelper_3.waypointsSearchHelper4.getRegistry().removeIf(predicate);
            BaritoneHelper_3.waypointsSearchHelper4.getRegistry().add(new WaypointsEnumSettingHelper(str3, orElse.getVec3d1303(), orElse.getString517(), orElse.getString518()));
            MixinMessageIndicatorHelper.do343(Text.literal("Waypoint ").append(str).append(" has been renamed to").append(str3), -1);
            return 1;
        }))))).then(Feature.literal("list").executes(commandContext6 -> {
            Text empty = Text.empty();
            ((MutableText) empty).append("Waypoints list: ");
            ((MutableText) empty).append(Texts.join(BaritoneHelper_3.waypointsSearchHelper4.getRegistry().stream().map((v0) -> {
                return v0.getText1879();
            }).toList(), Text.literal(", ")));
            MixinMessageIndicatorHelper.do343(empty, -1);
            return 1;
        }));
        literalArgumentBuilder.then(Feature.literal("toggle").then(Feature.argument("server", StringArgumentType.string()).suggests(Feature_25::getCompletableFuture2121).then(Feature.argument("name", StringArgumentType.string()).suggests(Feature_25::getCompletableFuture2123).executes(commandContext7 -> {
            String str = (String) commandContext7.getArgument("name", String.class);
            String str2 = (String) commandContext7.getArgument("server", String.class);
            WaypointsEnumSettingHelper orElse = BaritoneHelper_3.waypointsSearchHelper4.getOptional2404(waypointsEnumSettingHelper -> {
                return str2.equalsIgnoreCase(waypointsEnumSettingHelper.getString518()) && str.equalsIgnoreCase(waypointsEnumSettingHelper.getName());
            }).orElse(null);
            if (orElse == null) {
                MixinMessageIndicatorHelper.do343(Text.of("Waypoint not found"), -1);
                return 1;
            }
            orElse.do495(!orElse.isToggled());
            Object[] objArr = new Object[2];
            objArr[0] = str;
            objArr[1] = orElse.isToggled() ? "visible" : "invisible";
            MixinMessageIndicatorHelper.do343(Text.literal("Made the waypoint %s %s.".formatted(objArr)), -1);
            return 1;
        }))));
        if (BaritoneHelper_3.obstaclePasserHelper.is709()) {
            do415(new Feature_15(), literalArgumentBuilder);
            do415(new Feature_36(), literalArgumentBuilder);
        }
    }

    public static CompletableFuture<Suggestions> getCompletableFuture2121(CommandContext<CommandSource> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(BaritoneHelper_3.waypointsSearchHelper4.getRegistry().stream().map(Feature_25::getString2124).distinct(), suggestionsBuilder);
    }

    public static CompletableFuture<Suggestions> getCompletableFuture2122(CommandContext<CommandSource> commandContext, SuggestionsBuilder suggestionsBuilder, String str) {
        return CommandSource.suggestMatching(BaritoneHelper_3.waypointsSearchHelper4.getRegistry().stream().filter(waypointsEnumSettingHelper -> {
            return waypointsEnumSettingHelper.getString518().equalsIgnoreCase(str);
        }).map((v0) -> {
            return v0.getName();
        }), suggestionsBuilder);
    }

    public static CompletableFuture<Suggestions> getCompletableFuture2123(CommandContext<CommandSource> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return getCompletableFuture2122(commandContext, suggestionsBuilder, (String) commandContext.getArgument("server", String.class));
    }

    public static String getString2124(WaypointsEnumSettingHelper waypointsEnumSettingHelper) {
        String lowerCase = waypointsEnumSettingHelper.getString518().toLowerCase();
        return lowerCase.contains(":") ? new ArgumentTypeHelper().getArgumentTypeHelper2919(lowerCase).getString2921("\"\u0001\"") : lowerCase;
    }

    public static String getString2125() {
        ServerInfo serverInfo = minecraftClient.player.networkHandler.getServerInfo();
        if (serverInfo == null) {
            return "singleplayer";
        }
        String[] split = serverInfo.address.split(":");
        return (split.length == 2 && split[1].equalsIgnoreCase("25565")) ? split[0] : serverInfo.address;
    }

    public static Vec3d getVec3d2126(CommandContext<CommandSource> commandContext) {
        String str = (String) commandContext.getArgument("name", String.class);
        WaypointsEnumSettingHelper orElse = BaritoneHelper_3.waypointsSearchHelper4.getOptional2404(waypointsEnumSettingHelper -> {
            return waypointsEnumSettingHelper.is3068(str, getString2125());
        }).orElse(null);
        if (orElse == null) {
            return null;
        }
        return Waypoints.getVec3d2100(orElse);
    }
}
