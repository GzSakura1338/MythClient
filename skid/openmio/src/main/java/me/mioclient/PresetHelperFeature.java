package me.mioclient;

import me.mioclient.module.Module;
import net.minecraft.client.gui.screen.ConfirmScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelperFeature.class */
public final class PresetHelperFeature extends Feature {
    public static final String string = "ㅤ";
    public static boolean flag = false;

    public PresetHelperFeature() {
        super("panic");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.executes(commandContext -> {
            do413(() -> {
                minecraftClient.setScreen(new ConfirmScreen(z -> {
                    if (z) {
                        do2977();
                    }
                    minecraftClient.setScreen((Screen) null);
                }, Text.literal("Unload the client."), Text.literal("Continue?")));
            });
            return 1;
        });
    }

    public void do2977() {
        BaritoneHelper_3.presetHelper.do41();
        flag = true;
        String string2982 = ChatFilterSearchHelper4_2.getString2982();
        ChatFilterSearchHelper4_2.do2983("ㅤ");
        for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
            module.modifyKeybind(keybind -> {
                return keybind.getKeybind1941(-1);
            });
            if (module.isToggled()) {
                module.do496();
            }
        }
        minecraftClient.inGameHud.getChatHud().getMessageHistory().removeIf(str -> {
            return str.startsWith(string2982);
        });
        ((AntiSpamHelper) minecraftClient.inGameHud.getChatHud()).getVisible().removeIf(visible -> {
            SignatureHelper signatureHelper = (SignatureHelper)(Object) visible;
            return signatureHelper.getSignature() != null && signatureHelper.getSignature().toByteBuffer().getInt() < 0;
        });
        minecraftClient.inGameHud.getChatHud().getMessageHistory().removeIf(str2 -> {
            return str2.startsWith(string2982);
        });
    }
}
