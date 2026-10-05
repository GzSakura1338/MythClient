package me.mioclient;

import me.mioclient.module.exploit.IllegalDisconnect;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_18.class */
public final class Feature_18 extends Feature {
    public Feature_18() {
        super("disconnect");
        do414("dc");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("illegal").executes(commandContext -> {
            if (is1469()) {
                return 1;
            }
            IllegalDisconnect.do640();
            return 1;
        })).executes(commandContext2 -> {
            if (is1469()) {
                return 1;
            }
            minecraftClient.player.networkHandler.getConnection().disconnect(Text.of("Disconnected"));
            return 1;
        });
    }
}
