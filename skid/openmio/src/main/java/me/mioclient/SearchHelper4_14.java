package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.awt.Color;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import me.mioclient.module.client.Colors;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.entity.player.PlayerEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_14.class */
public final class SearchHelper4_14 extends StashFinderModuleListHelper<MixinPlayerEntityData> implements SearchHelper_4, PresetHelper_7 {
    public static final Colors colors = (Colors) BaritoneHelper_3.baritoneHelper_4.getModule117(Colors.class);
    public static final EnumSettingConverter enumSettingConverter = new EnumSettingConverter(NameTagsHelperMode.class);

    public boolean is519(String str) {
        return getNameTagsHelperMode525(str) == NameTagsHelperMode.FRIEND;
    }

    public boolean is520(PlayerEntity playerEntity) {
        return ((NameTagsHelper) playerEntity).mio$getRole() == NameTagsHelperMode.FRIEND;
    }

    public boolean is521(String str) {
        return getNameTagsHelperMode525(str) == NameTagsHelperMode.ENEMY;
    }

    public boolean is522(PlayerEntity playerEntity) {
        return ((NameTagsHelper) playerEntity).mio$getRole() == NameTagsHelperMode.ENEMY;
    }

    public void do523(String str) {
        is527(str);
        register(new MixinPlayerEntityData(str, NameTagsHelperMode.FRIEND));
    }

    public void do524(String str) {
        is527(str);
        register(new MixinPlayerEntityData(str, NameTagsHelperMode.ENEMY));
    }

    public NameTagsHelperMode getNameTagsHelperMode525(String str) {
        return (NameTagsHelperMode) getOptional2404(mixinPlayerEntityData -> {
            return mixinPlayerEntityData.getName().equalsIgnoreCase(str);
        }).map((v0) -> {
            return v0.getNameTagsHelperMode631();
        }).orElse(null);
    }

    public List<String> getList526(NameTagsHelperMode nameTagsHelperMode) {
        ArrayList arrayList = new ArrayList();
        synchronized (((List) this.registry)) {
            ((List<MixinPlayerEntityData>) this.registry).forEach(mixinPlayerEntityData -> {
                if (mixinPlayerEntityData.getNameTagsHelperMode631() == nameTagsHelperMode) {
                    arrayList.add(mixinPlayerEntityData.getName());
                }
            });
        }
        return arrayList;
    }

    public boolean is527(String str) {
        do531(str, (NameTagsHelperMode) null);
        return ((List<MixinPlayerEntityData>) this.registry).removeIf(mixinPlayerEntityData -> {
            return mixinPlayerEntityData.getName().equalsIgnoreCase(str);
        });
    }

    public Color getColor528() {
        return colors.friendColor.getValue();
    }

    public Color getColor529() {
        return colors.enemyColor.getValue();
    }

    public Color getColor530(String str, Color color) {
        return is519(str) ? getColor528() : is521(str) ? getColor529() : color;
    }

    public void do531(String str, NameTagsHelperMode nameTagsHelperMode) {
        if (is1469()) {
            return;
        }
        for (AbstractClientPlayerEntity nameTagsHelper : minecraftClient.world.getPlayers()) {
            if (nameTagsHelper.getGameProfile().getName().equalsIgnoreCase(str) && (nameTagsHelper instanceof NameTagsHelper)) {
                ((NameTagsHelper) (Object) nameTagsHelper).mio$setRole(nameTagsHelperMode);
                return;
            }
        }
    }

    @Override // me.mioclient.StashFinderModuleListHelper, me.mioclient.Helper_9
    /* renamed from: is532, reason: merged with bridge method [inline-methods] */
    public boolean register(MixinPlayerEntityData mixinPlayerEntityData) {
        do531(mixinPlayerEntityData.getString333(), mixinPlayerEntityData.getNameTagsHelperMode631());
        return super.register(mixinPlayerEntityData);
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        JsonArray jsonArray = new JsonArray();
        for (MixinPlayerEntityData mixinPlayerEntityData : (List<MixinPlayerEntityData>) this.registry) {
            JsonElement jsonObject2 = new JsonObject();
            ((JsonObject) jsonObject2).addProperty("name", mixinPlayerEntityData.getString333());
            ((JsonObject) jsonObject2).add("role", enumSettingConverter.doForward(mixinPlayerEntityData.getNameTagsHelperMode631()));
            jsonArray.add(jsonObject2);
        }
        jsonObject.add("socials", (JsonElement) jsonArray);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.getAsJsonObject().has("socials")) {
            Iterator it = jsonElement.getAsJsonObject().getAsJsonArray("socials").iterator();
            while (it.hasNext()) {
                JsonObject asJsonObject = ((JsonElement) it.next()).getAsJsonObject();
                if (asJsonObject.has("name") && asJsonObject.has("role")) {
                    register(new MixinPlayerEntityData(asJsonObject.get("name").getAsString(), (NameTagsHelperMode) enumSettingConverter.doBackward(asJsonObject.get("role"))));
                }
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "socials.json";
    }
}
