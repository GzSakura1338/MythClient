package me.mioclient;

import me.mioclient.module.player.RotationLock;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_7.class */
public class Feature_7 extends Feature {
    public static final RotationLock rotationLock = (RotationLock) BaritoneHelper_3.baritoneHelper_4.getModule117(RotationLock.class);

    public Feature_7() {
        super("angle");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("pos", new ExamplesArgumentType()).then(Feature.literal("yawlock").executes(commandContext -> {
            float[] floatArray2484 = SearchHelper4_8.getFloatArray2484((Vec3d) commandContext.getArgument("pos", Vec3d.class));
            rotationLock.value2.do2333(Float.valueOf(floatArray2484[0]));
            return 1;
        })).executes(commandContext2 -> {
            float[] floatArray2484 = SearchHelper4_8.getFloatArray2484((Vec3d) commandContext2.getArgument("pos", Vec3d.class));
            MixinMessageIndicatorHelper.do344(Text.literal("Calculated angle. Yaw: %.1f, Pitch: %.1f.".formatted(Float.valueOf(floatArray2484[0]), Float.valueOf(floatArray2484[1]))), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }));
    }
}
