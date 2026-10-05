package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import net.minecraft.client.gui.screen.TitleScreen;
import net.minecraft.client.gui.screen.multiplayer.ConnectScreen;
import net.minecraft.client.gui.screen.multiplayer.MultiplayerScreen;
import net.minecraft.client.network.CookieStorage;
import net.minecraft.client.network.ServerAddress;
import net.minecraft.client.network.ServerInfo;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_22.class */
public final class Feature_22 extends Feature {
    public Feature_22() {
        super("session");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("name", StringArgumentType.string()).executes(commandContext -> {
            String str = (String) commandContext.getArgument("name", String.class);
            if (str.length() > 16) {
                return 0;
            }
            if (minecraftClient.getNetworkHandler().getServerInfo() != null) {
                minecraftClient.getNetworkHandler().getConnection().disconnect(Text.of(""));
                ServerInfo serverInfo = minecraftClient.getNetworkHandler().getServerInfo();
                ConnectScreen.connect(new MultiplayerScreen(new TitleScreen()), minecraftClient, ServerAddress.parse(serverInfo.address), serverInfo, true, (CookieStorage) null);
            }
            ((me.mioclient.mixin.ducks.DuckSession) minecraftClient.getSession()).setUsername(str);
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("Session's name has been set to \u0001.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }));
    }
}
