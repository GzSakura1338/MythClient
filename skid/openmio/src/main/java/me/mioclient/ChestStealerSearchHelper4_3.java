package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import me.mioclient.ChestStealerEnumSettingHelper;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.component.type.PotionContentsComponent;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChestStealerSearchHelper4_3.class */
public final class ChestStealerSearchHelper4_3 extends StashFinderModuleListHelper<ChestStealerEnumSettingHelper> implements SearchHelper_4, PresetHelper_7 {
    public String string;

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        if (this.string != null) {
            jsonObject.addProperty("current", this.string);
        }
        for (ChestStealerEnumSettingHelper chestStealerEnumSettingHelper : getRegistry()) {
            JsonArray jsonArray = new JsonArray();
            chestStealerEnumSettingHelper.getMap2747().forEach((num, record) -> {
                JsonObject jsonObject2 = new JsonObject();
                jsonObject2.addProperty("id", Integer.valueOf(record.get2112()));
                if (record.getString2113() != null) {
                    jsonObject2.addProperty("data", record.getString2113());
                }
                JsonElement jsonObject3 = new JsonObject();
                ((JsonObject) jsonObject3).addProperty("slot", num);
                ((JsonObject) jsonObject3).add("item", (JsonElement) jsonObject2);
                jsonArray.add(jsonObject3);
            });
            jsonObject.add(chestStealerEnumSettingHelper.getName(), jsonArray);
        }
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.isJsonObject()) {
            JsonObject asJsonObject = jsonElement.getAsJsonObject();
            if (asJsonObject.has("current")) {
                do258(asJsonObject.get("current").getAsString());
            }
            for (Map.Entry entry : asJsonObject.entrySet()) {
                if (((JsonElement) entry.getValue()).isJsonArray()) {
                    ChestStealerEnumSettingHelper chestStealerEnumSettingHelper = new ChestStealerEnumSettingHelper((String) entry.getKey());
                    Iterator it = ((JsonElement) entry.getValue()).getAsJsonArray().iterator();
                    while (it.hasNext()) {
                        JsonElement jsonElement2 = (JsonElement) it.next();
                        if (jsonElement2.isJsonObject()) {
                            JsonObject asJsonObject2 = jsonElement2.getAsJsonObject();
                            if (asJsonObject2.has("slot") && asJsonObject2.has("item")) {
                                ChestStealerEnumSettingHelper.Record record = null;
                                JsonObject jsonObject = (asJsonObject2.get("item")) instanceof JsonObject ? (JsonObject) (asJsonObject2.get("item")) : null;
                                if (jsonObject instanceof JsonObject) {
                                    JsonObject jsonObject2 = jsonObject;
                                    if (jsonObject2.has("id")) {
                                        record = new ChestStealerEnumSettingHelper.Record(jsonObject2.get("id").getAsInt(), jsonObject2.has("data") ? jsonObject2.get("data").getAsString() : null);
                                    }
                                }
                                JsonPrimitive jsonPrimitive = (asJsonObject2.get("item")) instanceof JsonPrimitive ? (JsonPrimitive) (asJsonObject2.get("item")) : null;
                                if (jsonPrimitive instanceof JsonPrimitive) {
                                    JsonPrimitive jsonPrimitive2 = jsonPrimitive;
                                    if (jsonPrimitive2.isNumber()) {
                                        record = new ChestStealerEnumSettingHelper.Record(jsonPrimitive2.getAsInt(), null);
                                    }
                                }
                                if (record != null) {
                                    chestStealerEnumSettingHelper.getMap2747().put(Integer.valueOf(asJsonObject2.get("slot").getAsInt()), record);
                                }
                            }
                        }
                    }
                    register(chestStealerEnumSettingHelper);
                }
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "kits.json";
    }

    public ChestStealerEnumSettingHelper getChestStealerEnumSettingHelper257() {
        if (this.string == null) {
            return null;
        }
        return (ChestStealerEnumSettingHelper) getOptional2404(chestStealerEnumSettingHelper -> {
            return chestStealerEnumSettingHelper.getName().equalsIgnoreCase(this.string);
        }).orElse(null);
    }

    public void do258(String str) {
        this.string = str;
    }

    public void do40(String str) {
        getRegistry().removeIf(chestStealerEnumSettingHelper -> {
            return chestStealerEnumSettingHelper.getName().equalsIgnoreCase(str);
        });
        ChestStealerEnumSettingHelper chestStealerEnumSettingHelper2 = new ChestStealerEnumSettingHelper(str);
        int i = 0;
        Iterator it = minecraftClient.player.getInventory().main.iterator();
        while (it.hasNext()) {
            ItemStack itemStack = (ItemStack) it.next();
            chestStealerEnumSettingHelper2.getMap2747().put(Integer.valueOf(i), new ChestStealerEnumSettingHelper.Record(Item.getRawId(itemStack.getItem()), getString259(itemStack)));
            i++;
        }
        getRegistry().add(chestStealerEnumSettingHelper2);
        do258(str);
    }

    public static String getString259(ItemStack itemStack) {
        PotionContentsComponent potionContentsComponent = (PotionContentsComponent) itemStack.get(DataComponentTypes.POTION_CONTENTS);
        if (potionContentsComponent == null) {
            return null;
        }
        ArrayList<String> arrayList = new ArrayList<>();
        potionContentsComponent.forEachEffect(statusEffectInstance -> {
            List<String> list = arrayList;
            list.add(new ArgumentTypeHelper().getArgumentTypeHelper2906(statusEffectInstance.getAmplifier() + 1).getArgumentTypeHelper2919(statusEffectInstance.getTranslationKey()).getString2921("\u0001 \u0001"));
        });
        arrayList.sort(Comparator.comparing(str -> {
            return str;
        }));
        return String.join("", arrayList);
    }
}
