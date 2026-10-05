package me.mioclient;

import me.mioclient.api.Keybind;
import net.minecraft.client.option.KeyBinding;
import net.minecraft.client.util.InputUtil;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/EntityControlSearchHelper4.class */
public class EntityControlSearchHelper4 implements SearchHelper_4 {
    public static String getString2602(Keybind keybind) {
        return getString2603(keybind.get1945(), keybind.is1947());
    }

    public static String getString2603(int i, boolean z) {
        return i < 0 ? "NONE" : z ? new ArgumentTypeHelper().getArgumentTypeHelper2906(i).getString2921("MOUSE\u0001") : InputUtil.fromKeyCode(i, 0).getTranslationKey().replaceAll("key.keyboard.", "").replace(".", "_").toUpperCase();
    }

    public static boolean is2604(Keybind keybind) {
        return keybind.is1947() ? is2606(keybind.get1945()) : is2605(keybind.get1945());
    }

    public static boolean is1538(KeyBinding keyBinding) {
        InputUtil.Key key = keyBinding.boundKey;
        return key.getCategory() == InputUtil.Type.MOUSE ? is2606(key.getCode()) : is2605(key.getCode());
    }

    public static boolean is2605(int i) {
        return GLFW.glfwGetKey(minecraftClient.getWindow().getHandle(), i) == 1;
    }

    public static boolean is2606(int i) {
        return GLFW.glfwGetMouseButton(minecraftClient.getWindow().getHandle(), i) == 1;
    }

    public static boolean is2607() {
        return GLFW.glfwGetKey(minecraftClient.getWindow().getHandle(), 341) == 1;
    }

    public static boolean is2608() {
        return GLFW.glfwGetKey(minecraftClient.getWindow().getHandle(), 340) == 1;
    }

    public static boolean is2609() {
        return GLFW.glfwGetKey(minecraftClient.getWindow().getHandle(), 342) == 1;
    }
}
