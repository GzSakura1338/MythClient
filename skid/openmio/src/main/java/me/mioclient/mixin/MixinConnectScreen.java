package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.ConnectEvent;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.screen.multiplayer.ConnectScreen;
import net.minecraft.client.network.CookieStorage;
import net.minecraft.client.network.ServerAddress;
import net.minecraft.client.network.ServerInfo;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({ConnectScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinConnectScreen.class */
public class MixinConnectScreen {
    @Inject(method = {"connect(Lnet/minecraft/client/MinecraftClient;Lnet/minecraft/client/network/ServerAddress;Lnet/minecraft/client/network/ServerInfo;Lnet/minecraft/client/network/CookieStorage;)V"}, at = {@At("HEAD")})
    private void connectHook(MinecraftClient minecraftClient, ServerAddress serverAddress, ServerInfo serverInfo, CookieStorage cookieStorage, CallbackInfo callbackInfo) {
        ServerInfo serverInfo2 = serverInfo;
        if (serverInfo2 == null) {
            serverInfo2 = new ServerInfo("mioclient", serverAddress.getAddress(), ServerInfo.ServerType.OTHER);
        }
        SearchHelper_4.baritoneHelper.getObject1794(new ConnectEvent(serverInfo2.address));
        BaritoneHelper_3.holeSnapSearchHelper4_4.do2623(serverInfo2);
    }
}
