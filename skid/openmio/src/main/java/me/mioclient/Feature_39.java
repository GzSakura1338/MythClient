package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_39.class */
public final class Feature_39 extends Feature {
    public Feature_39() {
        super("irc");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("ping").then(Feature.argument("pos", new ExamplesArgumentType()).executes(commandContext -> {
            do3097(BlockPos.ofFloored((Vec3d) commandContext.getArgument("pos", Vec3d.class)));
            return 1;
        })).executes(commandContext2 -> {
            BlockHitResult blockHitResult = (minecraftClient.crosshairTarget) instanceof BlockHitResult ? (BlockHitResult) (minecraftClient.crosshairTarget) : null;
            if (!(blockHitResult instanceof BlockHitResult)) {
                return 1;
            }
            do3097(blockHitResult.getBlockPos());
            return 1;
        })).then(Feature.literal("help").executes(commandContext3 -> {
            int i = -8134;
            if (BaritoneHelper_3.nameTagsSearchHelper4.getStringArray2310() == null) {
                return 1;
            }
            for (String str : BaritoneHelper_3.nameTagsSearchHelper4.getStringArray2310()) {
                i--;
                MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getArgumentTypeHelper2919(ChatFilterSearchHelper4_2.getString2982()).getString2921("\u0001irc \u0001")), MixinMessageIndicatorHelper.getMessageSignatureData337(i));
            }
            return 1;
        }));
        do415(new Feature_26(), literalArgumentBuilder);
        if (is3098((CommandSource) null)) {
            literalArgumentBuilder.then(Feature.literal("online").requires(this::is3098).executes(commandContext4 -> {
                BaritoneHelper_3.nameTagsSearchHelper4.do2302();
                return 1;
            }));
            literalArgumentBuilder.then(Feature.literal("crash").requires(this::is3098).then(Feature.argument("name", StringArgumentType.word()).executes(commandContext5 -> {
                BaritoneHelper_3.nameTagsSearchHelper4.do2298((String) commandContext5.getArgument("name", String.class));
                return 1;
            })));
            literalArgumentBuilder.then(Feature.literal("ban").requires(this::is3098).then(Feature.argument("name", StringArgumentType.word()).executes(commandContext6 -> {
                BaritoneHelper_3.nameTagsSearchHelper4.do2299((String) commandContext6.getArgument("name", String.class));
                return 1;
            })));
            literalArgumentBuilder.then(Feature.literal("unban").requires(this::is3098).then(Feature.argument("name", StringArgumentType.word()).executes(commandContext7 -> {
                BaritoneHelper_3.nameTagsSearchHelper4.do2300((String) commandContext7.getArgument("name", String.class));
                return 1;
            })));
            literalArgumentBuilder.then(Feature.literal("players").requires(this::is3098).executes(commandContext8 -> {
                BaritoneHelper_3.nameTagsSearchHelper4.do2301();
                return 1;
            }));
        }
    }

    public void do3097(BlockPos blockPos) {
        String str = null;
        try {
            str = minecraftClient.getNetworkHandler().getConnection().getAddress().toString();
            int indexOf = str.indexOf(47);
            if (indexOf > 0) {
                str = str.substring(0, indexOf);
            }
            while (str.endsWith(".")) {
                str = str.substring(0, str.length() - 1);
            }
        } catch (Exception e) {
        }
        if (str == null || str.isEmpty()) {
            str = "singleplayer";
        }
        BaritoneHelper_3.nameTagsSearchHelper4.do2303(BaritoneHelper_3.welcomerHelper.getString2810(), str, blockPos);
    }

    public boolean is3098(CommandSource commandSource) {
        return BaritoneHelper_3.welcomerHelper.is2812();
    }
}
