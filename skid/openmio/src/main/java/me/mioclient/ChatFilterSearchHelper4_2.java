package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.mojang.brigadier.CommandDispatcher;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.tree.CommandNode;
import com.mojang.brigadier.tree.LiteralCommandNode;
import java.util.Iterator;
import java.util.List;
import me.mioclient.event.CharEvent;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import me.mioclient.module.client.UI;
import net.minecraft.client.gui.screen.ChatScreen;
import net.minecraft.client.network.ClientCommandSource;
import net.minecraft.client.network.ClientPlayNetworkHandler;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChatFilterSearchHelper4_2.class */
public final class ChatFilterSearchHelper4_2 extends StashFinderModuleListHelper<Feature> implements SearchHelper_4, PresetHelper_7 {
    public static UI clickgui = (UI) BaritoneHelper_3.baritoneHelper_4.getModule117(UI.class);
    public static final CommandSource commandSource = new ClientCommandSource((ClientPlayNetworkHandler) null, minecraftClient);
    public static final CommandDispatcher<CommandSource> commandDispatcher = new CommandDispatcher<>();
    public static String string = ";";

    public ChatFilterSearchHelper4_2() {
        baritoneHelper.do1796(this);
        PhaseESPHelper.do1353(this);
        do2985();
        do2987();
        do2986();
    }

    public static String getString2982() {
        return string;
    }

    public static void do2983(String str) {
        if (str.isBlank()) {
            return;
        }
        String substring = str.substring(0, 1);
        string = substring;
        clickgui.prefix.do2333(substring);
    }

    public static char get2984() {
        return string.charAt(0);
    }

    public void do2985() {
        if (BaritoneHelper_3.obstaclePasserHelper.is709()) {
            register(new Feature_35());
        }
    }

    public void do2986() {
        for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
            if (!getOptional2404(feature -> {
                return feature.getName().equalsIgnoreCase(module.getName());
            }).isPresent()) {
                for (String str : module.getAliases()) {
                    LiteralArgumentBuilder_2 literalArgumentBuilder_2 = new LiteralArgumentBuilder_2(str.toLowerCase());
                    new Feature_32(module).exec(literalArgumentBuilder_2);
                    commandDispatcher.register(literalArgumentBuilder_2);
                }
            }
        }
    }

    public static void do2060(String str) {
        StringEvent stringEvent = new StringEvent(str);
        SearchHelper_4.baritoneHelper.getObject1794(stringEvent);
        if (stringEvent.is2403()) {
            return;
        }
        try {
            commandDispatcher.execute(commandDispatcher.parse(str, commandSource));
        } catch (CommandSyntaxException e) {
        }
    }

    public void do2987() {
        register(new Feature_10(NameTagsHelperMode.FRIEND));
        register(new Feature_10(NameTagsHelperMode.ENEMY));
    }

    @Override // me.mioclient.StashFinderModuleListHelper, me.mioclient.Helper_9
    /* renamed from: is2988, reason: merged with bridge method [inline-methods] */
    public boolean register(Feature feature) {
        com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literal = com.mojang.brigadier.builder.LiteralArgumentBuilder.literal(feature.getName());
        feature.exec(literal);
        LiteralCommandNode register = commandDispatcher.register(literal);
        for (String str : feature.getAliases()) {
            LiteralArgumentBuilder literalArgumentBuilder = new LiteralArgumentBuilder(str);
            Iterator it = register.getChildren().iterator();
            while (it.hasNext()) {
                literalArgumentBuilder.then((CommandNode) it.next());
            }
            literalArgumentBuilder.executes(commandContext -> {
                return register.getCommand().run(commandContext);
            });
            commandDispatcher.register(literalArgumentBuilder);
        }
        return ((List) this.registry).add(feature);
    }

    @Listen
    public void onChar(CharEvent charEvent) {
        if (getString2982().length() == 1 && getString2982().charAt(0) == charEvent.get1446()) {
            minecraftClient.setScreen(new ChatScreen(""));
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("prefix", string);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.getAsJsonObject().has("prefix")) {
            do2983(jsonElement.getAsJsonObject().get("prefix").getAsString());
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "commands.json";
    }
}
