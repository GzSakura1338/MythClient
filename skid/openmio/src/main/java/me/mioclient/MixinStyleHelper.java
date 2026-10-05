package me.mioclient;

import java.util.function.Supplier;
import org.jetbrains.annotations.Nullable;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MixinStyleHelper.class */
public interface MixinStyleHelper {
    @Nullable
    Supplier<Integer> getSupplier();

    void setSupplier(@Nullable Supplier<Integer> supplier);
}
