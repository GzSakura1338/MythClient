package me.mioclient.module;

import java.util.Objects;
import me.mioclient.EnumSettingHelper;
import me.mioclient.SearchHelper_4;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Feature.class */
public class Feature implements SearchHelper_4, EnumSettingHelper {
    public final String name;
    public String description = "";

    public Feature(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public String getDescription() {
        return this.description;
    }

    public void setDescription(String str) {
        this.description = str;
    }

    public boolean isDescriptionPresent() {
        return !Objects.equals(this.description, "");
    }

    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || getClass() != obj.getClass()) {
            return false;
        }
        return Objects.equals(this.name, ((Feature) obj).name);
    }

    public int hashCode() {
        return Objects.hash(this.name);
    }
}
