package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/DiscordNotifsHelper.class */
public class DiscordNotifsHelper implements SearchHelper_4, PresetHelper_7 {
    public String string = "";

    public DiscordNotifsHelper() {
        baritoneHelper.do1796(this);
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("url", this.string);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        this.string = jsonElement.getAsJsonObject().get("url").getAsString();
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "webhook.json";
    }

    public void do1199(String str) {
        this.string = str;
    }

    public String getString1200() {
        return this.string;
    }

    public void do1201(String str) {
        executorService.submit(() -> {
            try {
                JsonElement jsonObject = new JsonObject();
                ((JsonObject) jsonObject).addProperty("content", str);
                HttpClient.newHttpClient().send(DiscordNotifsHelperSearchHelper4.getBuilder2964(BaritoneHelper_3.discordNotifsHelper.getString1200(), HttpRequest.BodyPublishers.ofString(SearchHelper_4.gson.toJson(jsonObject))).header("User-Agent", "MioClient/2.0").headers(new String[]{"Content-Type", "application/json"}).build(), HttpResponse.BodyHandlers.ofString());
            } catch (Exception e) {
            }
        });
    }
}
