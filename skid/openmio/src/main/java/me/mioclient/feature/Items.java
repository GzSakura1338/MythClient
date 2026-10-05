package me.mioclient.feature;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EntityListObjectSetting;
import me.mioclient.PresetHelper_7;
import net.minecraft.item.Item;
import net.minecraft.registry.Registries;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Items.class */
public class Items implements PresetHelper_7 {
    public final EntityListObjectSetting<Item> entityListObjectSetting = new EntityListObjectSetting<>("Items", Registries.ITEM, new Item[0]);

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        jsonObject.add("items", this.entityListObjectSetting.toJson());
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement instanceof JsonObject) {
            JsonObject jsonObject = (JsonObject) jsonElement;
            if (jsonObject.has("items")) {
                this.entityListObjectSetting.fromJson(jsonObject.get("items"));
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "glint.json";
    }

    public EntityListObjectSetting<Item> getEntityListObjectSetting1123() {
        return this.entityListObjectSetting;
    }

    public static boolean is1124(Item item) {
        if (BaritoneHelper_3.items == null) {
            return false;
        }
        return BaritoneHelper_3.items.entityListObjectSetting.getValue().contains(item);
    }
}
