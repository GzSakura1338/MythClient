package me.mioclient;

import java.io.IOException;
import java.nio.file.Files;
import java.util.HashSet;
import java.util.concurrent.CompletableFuture;
import net.minecraft.client.texture.ResourceTexture;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;
import net.minecraft.util.Identifier;
import me.mioclient.mixin.ducks.DuckTextureManager;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/CompletableFutureFeature_2.class */
public class CompletableFutureFeature_2 extends Feature {
    public static final byte[] byteArr = {-119, 80, 78, 71, 13, 10, 26, 10};
    public CompletableFuture<Void> completableFuture;

    public CompletableFutureFeature_2() {
        super("reload");
        this.completableFuture = CompletableFuture.completedFuture(null);
        do414("rl");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.executes(commandContext -> {
            if (!this.completableFuture.isDone()) {
                return 0;
            }
            MixinMessageIndicatorHelper.do344(Text.literal("Reloading Mio resources..."), MixinMessageIndicatorHelper.getMessageSignatureData338(this));
            this.completableFuture = CompletableFuture.runAsync(() -> {
                HashSet<Identifier> hashSet = new HashSet<>();
                for (Identifier identifier : ((DuckTextureManager) minecraftClient.getTextureManager()).getAllTextures().keySet()) {
                    if (identifier.getNamespace().equals("mio-mount")) {
                        hashSet.add(identifier);
                    }
                }
                BaritoneHelper_3.presetHelper.do71();
                hashSet.forEach(identifier2 -> {
                    byte[] readAllBytes;
                    try {
                        readAllBytes = Files.readAllBytes(PresetHelper.path.resolve(identifier2.getPath()));
                    } catch (IOException | java.lang.RuntimeException e) {
                        MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(e.getMessage()).getString2921("Invalid Image! \u0001")).styled(style -> {
                            return style.withFormatting(Formatting.RED);
                        }), MixinMessageIndicatorHelper.getMessageSignatureData337(-999));
                        return;
                    }
                    if (readAllBytes.length < 8) {
                        throw new java.lang.RuntimeException("File size is too small to be png");
                    }
                    for (int i = 0; i < 8; i++) {
                        if (readAllBytes[i] != byteArr[i]) {
                            throw new java.lang.RuntimeException("File is not a png");
                        }
                    }
                    minecraftClient.getTextureManager().destroyTexture(identifier2);
                    minecraftClient.getTextureManager().registerTexture(identifier2, new ResourceTexture(identifier2));
                });
                BaritoneHelper_3.searchHelper4_11.do2973();
            }, executorService);
            this.completableFuture.whenComplete((r4, th) -> {
                MixinMessageIndicatorHelper.do344(Text.literal("Reload complete!").styled(style -> {
                    return style.withFormatting(Formatting.GREEN);
                }), MixinMessageIndicatorHelper.getMessageSignatureData338(this));
            });
            return 1;
        });
    }
}
