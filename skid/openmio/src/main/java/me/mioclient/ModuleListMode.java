package me.mioclient;

import java.util.function.Supplier;
import net.minecraft.util.math.MathHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ModuleListMode.class */
public enum ModuleListMode implements SearchHelper_4, EnumSettingHelper {
    TOP_LEFT("top_left", () -> {
        return Data_3.getData_31612(0.0f, 0.0f, 10.0f, 10.0f);
    }),
    TOP_RIGHT("top_right", () -> {
        return Data_3.getData_31612(minecraftClient.getWindow().getScaledWidth() - 10, 0.0f, 10.0f, 10.0f);
    }),
    BOTTOM_LEFT("bottom_left", () -> {
        return Data_3.getData_31612(0.0f, minecraftClient.getWindow().getScaledHeight() - 10, 10.0f, 10.0f);
    }),
    BOTTOM_RIGHT("bottom_right", () -> {
        return Data_3.getData_31612(minecraftClient.getWindow().getScaledWidth() - 10, minecraftClient.getWindow().getScaledHeight() - 10, 10.0f, 10.0f);
    }),
    TOP_CENTER("top_center", () -> {
        return Data_3.getData_31612((minecraftClient.getWindow().getScaledWidth() / 2.0f) - 10.0f, 0.0f, 10.0f, 10.0f);
    }),
    NONE("none", () -> {
        return Data_3.getData_31612(0.0f, 0.0f, minecraftClient.getWindow().getScaledWidth(), minecraftClient.getWindow().getScaledHeight());
    });

    public final String name;
    public final Supplier<Data_3> supplier;

    ModuleListMode(String str, Supplier supplier) {
        this.name = str;
        this.supplier = supplier;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public boolean is92(double d, double d2) {
        return this.supplier.get().is92(MathHelper.clamp(d, 0.0d, minecraftClient.getWindow().getScaledWidth()), MathHelper.clamp(d2, 0.0d, minecraftClient.getWindow().getScaledHeight()));
    }

    public Data_3 getData_32936() {
        return this.supplier.get();
    }
}
