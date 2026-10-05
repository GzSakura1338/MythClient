package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.Helper_7;
import me.mioclient.module.misc.AutoReconnect;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.DisconnectedScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.TitleScreen;
import net.minecraft.client.gui.screen.multiplayer.ConnectScreen;
import net.minecraft.client.gui.screen.multiplayer.MultiplayerScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.DirectionalLayoutWidget;
import net.minecraft.client.network.CookieStorage;
import net.minecraft.client.network.ServerAddress;
import net.minecraft.client.network.ServerInfo;
import net.minecraft.network.DisconnectionInfo;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({DisconnectedScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinDisconnectedScreen.class */
public class MixinDisconnectedScreen extends Screen {
    private static AutoReconnect autoreconnect = (AutoReconnect) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoReconnect.class);

    @Unique
    private long startTime;

    @Shadow
    @Final
    private DisconnectionInfo field_52131;

    @Shadow
    @Final
    private DirectionalLayoutWidget field_44552;

    @Unique
    private ButtonWidget reconnectButton;

    @Unique
    private ButtonWidget autoReconnectButton;

    protected MixinDisconnectedScreen(Text text) {
        super(text);
    }

    @Inject(method = {"init"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/screen/DisconnectedScreen;initTabNavigation()V", shift = At.Shift.BEFORE)})
    private void initHook(CallbackInfo callbackInfo) {
        this.startTime = System.currentTimeMillis();
        ButtonWidget.Builder width = ButtonWidget.builder(Text.literal("Reconnect"), buttonWidget -> {
            ServerInfo serverInfo2622 = BaritoneHelper_3.holeSnapSearchHelper4_4.getServerInfo2622();
            if (serverInfo2622 != null) {
                ConnectScreen.connect(new MultiplayerScreen(new TitleScreen()), this.client, ServerAddress.parse(serverInfo2622.address), serverInfo2622, true, (CookieStorage) null);
            }
        }).width(Helper_7.num);
        ButtonWidget.Builder width2 = ButtonWidget.builder(Text.literal("AutoReconnect"), buttonWidget2 -> {
            if (!autoreconnect.isToggled()) {
                BaritoneHelper_3.holeSnapSearchHelper4_4.flag3 = false;
            }
            autoreconnect.do496();
            this.startTime = System.currentTimeMillis();
            buttonWidget2.setFocused(false);
        }).width(Helper_7.num);
        this.reconnectButton = width.build();
        this.autoReconnectButton = width2.build();
        this.field_44552.refreshPositions();
        addDrawableChild(this.reconnectButton);
        addDrawableChild(this.autoReconnectButton);
    }

    @Inject(method = {"initTabNavigation"}, at = {@At("TAIL")})
    private void initTabHook(CallbackInfo callbackInfo) {
        if (this.reconnectButton == null) {
            return;
        }
        int i = (this.width / 2) - 100;
        this.reconnectButton.setPosition(i, Math.min((this.height / 2) + (this.field_44552.getHeight() / 2), this.height - 30) - 1);
        this.autoReconnectButton.setPosition(i, Math.min((this.height / 2) + (this.field_44552.getHeight() / 2), this.height - 30) + 20);
    }

    public void tick() {
        ServerInfo serverInfo2622;
        String string = this.field_52131.reason().getString();
        if (string.contains("Mio") && string.contains("[AutoLog]")) {
            this.autoReconnectButton.setX(-1000);
            this.autoReconnectButton.setY(-1000);
        } else if (autoreconnect.isToggled() && getAutoReconnectTime(autoreconnect) <= 0.0f && (serverInfo2622 = BaritoneHelper_3.holeSnapSearchHelper4_4.getServerInfo2622()) != null) {
            ConnectScreen.connect(new MultiplayerScreen(new TitleScreen()), this.client, ServerAddress.parse(serverInfo2622.address), serverInfo2622, true, (CookieStorage) null);
        }
    }

    public void render(DrawContext drawContext, int i, int i2, float f) {
        String string = this.field_52131.reason().getString();
        if (string.contains("Mio") && string.contains("[AutoLog]")) {
            this.autoReconnectButton.setX(-1000);
            this.autoReconnectButton.setY(-1000);
        } else {
            this.autoReconnectButton.setMessage(getAutoReconnectText());
            this.autoReconnectButton.setFocused(false);
            super.render(drawContext, i, i2, f);
        }
    }

    private Text getAutoReconnectText() {
        MutableText literal = Text.literal("AutoReconnect ");
        String string = this.field_52131.reason().getString();
        return BaritoneHelper_3.holeSnapSearchHelper4_4.flag3 ? literal.append(String.valueOf(Formatting.RED) + "AutoLogged") : autoreconnect.isToggled() && (!string.contains("Mio") || !string.contains("[AutoLog]")) ? literal.append(String.valueOf(Formatting.GREEN) + "%.1fs".formatted(Float.valueOf(getAutoReconnectTime(autoreconnect)))) : literal.append(String.valueOf(Formatting.RED) + "OFF");
    }

    private float getAutoReconnectTime(AutoReconnect autoReconnect) {
        if (autoReconnect == null) {
            return 0.1f;
        }
        return Math.max(autoReconnect.delay.getValue().floatValue() - (((float) (System.currentTimeMillis() - this.startTime)) / 1000.0f), 0.0f);
    }
}
