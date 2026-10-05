package com.jagrosh.discordipc.entities;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/RichPresence.class */
public class RichPresence {
    public final String state;
    public final String details;
    public final long startTimestamp;
    public final long endTimestamp;
    public final String largeImageKey;
    public final String largeImageText;
    public final String smallImageKey;
    public final String smallImageText;
    public final String partyId;
    public final int partySize;
    public final int partyMax;
    public final int partyPrivacy;
    public final String matchSecret;
    public final String joinSecret;
    public final String spectateSecret;
    public final JsonArray buttons;
    public final boolean instance;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/RichPresence$Builder.class */
    public static class Builder {
        public String state;
        public String details;
        public long startTimestamp;
        public long endTimestamp;
        public String largeImageKey;
        public String largeImageText;
        public String smallImageKey;
        public String smallImageText;
        public String partyId;
        public int partySize;
        public int partyMax;
        public int partyPrivacy;
        public String matchSecret;
        public String joinSecret;
        public String spectateSecret;
        public JsonArray buttons;
        public boolean instance;

        public RichPresence build() {
            return new RichPresence(this.state, this.details, this.startTimestamp, this.endTimestamp, this.largeImageKey, this.largeImageText, this.smallImageKey, this.smallImageText, this.partyId, this.partySize, this.partyMax, this.partyPrivacy, this.matchSecret, this.joinSecret, this.spectateSecret, this.buttons, this.instance);
        }

        public Builder setState(String str) {
            this.state = str;
            return this;
        }

        public Builder setDetails(String str) {
            this.details = str;
            return this;
        }

        public Builder setStartTimestamp(long j) {
            this.startTimestamp = j;
            return this;
        }

        public Builder setEndTimestamp(long j) {
            this.endTimestamp = j;
            return this;
        }

        public Builder setLargeImage(String str, String str2) {
            this.largeImageKey = str;
            this.largeImageText = str2;
            return this;
        }

        public Builder setLargeImage(String str) {
            return setLargeImage(str, null);
        }

        public Builder setSmallImage(String str, String str2) {
            this.smallImageKey = str;
            this.smallImageText = str2;
            return this;
        }

        public Builder setSmallImage(String str) {
            return setSmallImage(str, null);
        }

        public Builder setParty(String str, int i, int i2, int i3) {
            this.partyId = str;
            this.partySize = i;
            this.partyMax = i2;
            this.partyPrivacy = i3;
            return this;
        }

        public Builder setMatchSecret(String str) {
            this.matchSecret = str;
            return this;
        }

        public Builder setJoinSecret(String str) {
            this.joinSecret = str;
            return this;
        }

        public Builder setSpectateSecret(String str) {
            this.spectateSecret = str;
            return this;
        }

        public Builder setButtons(JsonArray jsonArray) {
            this.buttons = jsonArray;
            return this;
        }

        public Builder setInstance(boolean z) {
            this.instance = z;
            return this;
        }
    }

    public RichPresence(String str, String str2, long j, long j2, String str3, String str4, String str5, String str6, String str7, int i, int i2, int i3, String str8, String str9, String str10, JsonArray jsonArray, boolean z) {
        this.state = str;
        this.details = str2;
        this.startTimestamp = j;
        this.endTimestamp = j2;
        this.largeImageKey = str3;
        this.largeImageText = str4;
        this.smallImageKey = str5;
        this.smallImageText = str6;
        this.partyId = str7;
        this.partySize = i;
        this.partyMax = i2;
        this.partyPrivacy = i3;
        this.matchSecret = str8;
        this.joinSecret = str9;
        this.spectateSecret = str10;
        this.buttons = jsonArray;
        this.instance = z;
    }

    public JsonObject toJson() {
        JsonObject jsonObject = new JsonObject();
        JsonObject jsonObject2 = new JsonObject();
        JsonObject jsonObject3 = new JsonObject();
        JsonObject jsonObject4 = new JsonObject();
        JsonObject jsonObject5 = new JsonObject();
        if (this.startTimestamp > 0) {
            jsonObject.addProperty("start", Long.valueOf(this.startTimestamp));
            if (this.endTimestamp > this.startTimestamp) {
                jsonObject.addProperty("end", Long.valueOf(this.endTimestamp));
            }
        }
        if (this.largeImageKey != null && !this.largeImageKey.isEmpty()) {
            jsonObject2.addProperty("large_image", this.largeImageKey);
            if (this.largeImageText != null && !this.largeImageText.isEmpty()) {
                jsonObject2.addProperty("large_text", this.largeImageText);
            }
        }
        if (this.smallImageKey != null && !this.smallImageKey.isEmpty()) {
            jsonObject2.addProperty("small_image", this.smallImageKey);
            if (this.smallImageText != null && !this.smallImageText.isEmpty()) {
                jsonObject2.addProperty("small_text", this.smallImageText);
            }
        }
        if ((this.partyId != null && !this.partyId.isEmpty()) || ((this.partySize > 0 && this.partyMax > 0) || this.partyPrivacy >= 0)) {
            if (this.partyId != null && !this.partyId.isEmpty()) {
                jsonObject3.addProperty("id", this.partyId);
            }
            JsonArray jsonArray = new JsonArray();
            if (this.partySize > 0) {
                jsonArray.add(new JsonPrimitive(Integer.valueOf(this.partySize)));
                if (this.partyMax >= this.partySize) {
                    jsonArray.add(new JsonPrimitive(Integer.valueOf(this.partyMax)));
                }
            }
            jsonObject3.add("size", jsonArray);
            if (this.partyPrivacy >= 0) {
                jsonObject3.add("privacy", new JsonPrimitive(Integer.valueOf(this.partyPrivacy)));
            }
        }
        if (this.joinSecret != null && !this.joinSecret.isEmpty()) {
            jsonObject4.addProperty("join", this.joinSecret);
        }
        if (this.spectateSecret != null && !this.spectateSecret.isEmpty()) {
            jsonObject4.addProperty("spectate", this.spectateSecret);
        }
        if (this.matchSecret != null && !this.matchSecret.isEmpty()) {
            jsonObject4.addProperty("match", this.matchSecret);
        }
        if (this.state != null && !this.state.isEmpty()) {
            jsonObject5.addProperty("state", this.state);
        }
        if (this.details != null && !this.details.isEmpty()) {
            jsonObject5.addProperty("details", this.details);
        }
        if (jsonObject.has("start")) {
            jsonObject5.add("timestamps", jsonObject);
        }
        if (jsonObject2.has("large_image")) {
            jsonObject5.add("assets", jsonObject2);
        }
        if (jsonObject3.has("id")) {
            jsonObject5.add("party", jsonObject3);
        }
        if (jsonObject4.has("join") || jsonObject4.has("spectate") || jsonObject4.has("match")) {
            jsonObject5.add("secrets", jsonObject4);
        }
        if (this.buttons != null && !this.buttons.isJsonNull() && this.buttons.size() > 0 && this.buttons.size() < 3) {
            jsonObject5.add("buttons", this.buttons);
        }
        jsonObject5.addProperty("instance", Boolean.valueOf(this.instance));
        return jsonObject5;
    }

    public String toDecodedJson(String str) {
        try {
            return new String(toJson().toString().getBytes(str));
        } catch (Exception e) {
            return toJson().toString();
        }
    }
}
