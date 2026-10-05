package me.mioclient.mixin;

import net.minecraft.client.WindowEventHandler;
import net.minecraft.client.WindowSettings;
import net.minecraft.client.util.MonitorTracker;
import net.minecraft.client.util.Window;
import org.lwjgl.glfw.GLFW;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({Window.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinWindow.class */
public class MixinWindow {
    @Inject(method = {"<init>"}, at = {@At(value = "INVOKE_ASSIGN", target = "Lorg/lwjgl/glfw/GLFW;glfwCreateWindow(IILjava/lang/CharSequence;JJ)J", shift = At.Shift.BEFORE, remap = false)})
    private void init(WindowEventHandler windowEventHandler, MonitorTracker monitorTracker, WindowSettings windowSettings, String str, String str2, CallbackInfo callbackInfo) {
        GLFW.glfwWindowHint(139270, 0);
    }
}
