package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Optional;
import java.util.stream.Stream;
import me.mioclient.module.misc.ChatFilter;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChatFilterSearchHelper4.class */
public class ChatFilterSearchHelper4 implements SearchHelper_4, PresetHelper_7 {
    public static ChatFilter chatFilter = (ChatFilter) BaritoneHelper_3.baritoneHelper_4.getModule117(ChatFilter.class);
    public final List<Data> list = Collections.synchronizedList(new ArrayList());

    public ChatFilterSearchHelper4() {
        baritoneHelper.do1796(this);
    }

    public boolean is2675(String str) {
        synchronized (this.list) {
            String lowerCase = str.toLowerCase();
            Iterator<Data> it = this.list.iterator();
            while (it.hasNext()) {
                if (it.next().getString1093().toLowerCase().equals(lowerCase)) {
                    return true;
                }
            }
            return false;
        }
    }

    public void do2676(String str, String str2) {
        synchronized (this.list) {
            this.list.add(new Data(str, str2));
        }
    }

    public boolean is2677(String str) {
        if (!chatFilter.caseSensitive.getValue().booleanValue()) {
            str = str.toLowerCase();
        }
        synchronized (this.list) {
            Iterator<Data> it = this.list.iterator();
            while (it.hasNext()) {
                String string1094 = it.next().getString1094();
                if (!chatFilter.caseSensitive.getValue().booleanValue()) {
                    string1094 = string1094.toLowerCase();
                }
                if (str.contains(string1094)) {
                    return true;
                }
            }
            return false;
        }
    }

    public boolean isEmpty() {
        boolean isEmpty;
        synchronized (this.list) {
            isEmpty = this.list.isEmpty();
        }
        return isEmpty;
    }

    public Stream<String> getStream2678() {
        Stream map;
        synchronized (this.list) {
            map = this.list.stream().map((v0) -> {
                return v0.getString1093();
            });
        }
        return map;
    }

    public Optional<Data> getOptional2679(String str) {
        Optional<Data> findAny;
        synchronized (this.list) {
            findAny = this.list.stream().filter(data -> {
                return data.getString1093().equalsIgnoreCase(str);
            }).findAny();
        }
        return findAny;
    }

    public void do2680(Data data) {
        synchronized (this.list) {
            this.list.remove(data);
        }
    }

    public int get2681() {
        int size;
        synchronized (this.list) {
            size = this.list.size();
        }
        return size;
    }

    public List<String> getList2682() {
        ArrayList arrayList;
        synchronized (this.list) {
            arrayList = new ArrayList();
            for (Data data : this.list) {
                arrayList.add("%s%s%s: \"%s\"".formatted(Formatting.GRAY, data.getString1093(), Formatting.RESET, data.getString1094()));
            }
        }
        return arrayList;
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        JsonArray jsonArray = new JsonArray();
        synchronized (this.list) {
            for (Data data : this.list) {
                JsonElement jsonObject2 = new JsonObject();
                ((JsonObject) jsonObject2).addProperty("id", data.getString1093());
                ((JsonObject) jsonObject2).addProperty("filter", data.getString1094());
                jsonArray.add(jsonObject2);
            }
        }
        jsonObject.add("filters", (JsonElement) jsonArray);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement instanceof JsonObject) {
            JsonObject jsonObject = (JsonObject) jsonElement;
            if (jsonObject.has("filters")) {
                JsonArray jsonArray = (jsonObject.get("filters")) instanceof JsonArray ? (JsonArray) (jsonObject.get("filters")) : null;
                if (jsonArray instanceof JsonArray) {
                    JsonArray jsonArray2 = jsonArray;
                    for (int i = 0; i < jsonArray2.size(); i++) {
                        JsonObject asJsonObject = jsonArray2.get(i).getAsJsonObject();
                        if (asJsonObject.has("id") && asJsonObject.has("filter")) {
                            String asString = asJsonObject.get("id").getAsString();
                            String asString2 = asJsonObject.get("filter").getAsString();
                            if (!is2675(asString)) {
                                do2676(asString, asString2);
                            }
                        }
                    }
                }
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "chatfilter.json";
    }
}
