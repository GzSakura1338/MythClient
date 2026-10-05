package dev.myth.utils.wrapper;

import net.minecraft.client.Minecraft;

/** Shared client reference used by the imported Starain utilities. */
public interface Wrapper {
    Minecraft mc = Minecraft.getInstance();
}
