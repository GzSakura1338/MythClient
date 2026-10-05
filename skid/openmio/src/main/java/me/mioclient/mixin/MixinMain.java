package me.mioclient.mixin;

import net.minecraft.client.main.Main;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({Main.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinMain.class */
public class MixinMain {
    @Inject(method = {"main*"}, at = {@At("HEAD")}, remap = false)
    private static void mainHook(CallbackInfo callbackInfo) {
        // macOS: 非 headless AWT 会与 GLFW 的 -XstartOnFirstThread 争抢 Cocoa 主线程
        // 事件循环，导致 glfwPollEvents 死锁（进世界后卡在 LOADING TERRAIN、clickgui 无法打开）。
        // 强制 headless=true —— Fonts 模块的 java.awt 字体光栅化(离屏 BufferedImage)在
        // headless 下照常工作。其他平台保持作者原本的 false。
        boolean mac = System.getProperty("os.name", "").toLowerCase().contains("mac");
        System.setProperty("java.awt.headless", mac ? "true" : "false");
    }
}
