package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_10.class */
public class ArgumentType_10 implements com.mojang.brigadier.arguments.ArgumentType<WaypointsEnumSettingHelper> {
    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public WaypointsEnumSettingHelper parse(StringReader stringReader) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        String readString = stringReader.readString();
        for (WaypointsEnumSettingHelper waypointsEnumSettingHelper : BaritoneHelper_3.waypointsSearchHelper4.getRegistry()) {
            if (waypointsEnumSettingHelper != null && waypointsEnumSettingHelper.getName() != null && waypointsEnumSettingHelper.getName().equalsIgnoreCase(readString)) {
                return waypointsEnumSettingHelper;
            }
        }
        throw new DynamicCommandExceptionType(obj -> {
            return Text.literal(String.format("Waypoint %s doesn't exists", obj));
        }).create(readString);
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        ArrayList arrayList = new ArrayList();
        for (WaypointsEnumSettingHelper waypointsEnumSettingHelper : BaritoneHelper_3.waypointsSearchHelper4.getRegistry()) {
            if (waypointsEnumSettingHelper != null && (waypointsEnumSettingHelper.getString518() == null || waypointsEnumSettingHelper.getString517() == null || waypointsEnumSettingHelper.getName() == null)) {
                arrayList.add(waypointsEnumSettingHelper);
            }
        }
        Iterator it = arrayList.iterator();
        while (it.hasNext()) {
            BaritoneHelper_3.waypointsSearchHelper4.unregister((WaypointsEnumSettingHelper) it.next());
        }
        return CommandSource.suggestMatching(BaritoneHelper_3.waypointsSearchHelper4.getRegistry().stream().map((v0) -> {
            return v0.getName();
        }), suggestionsBuilder);
    }
}
