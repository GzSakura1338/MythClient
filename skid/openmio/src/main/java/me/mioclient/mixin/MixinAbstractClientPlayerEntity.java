package me.mioclient.mixin;

import com.mojang.authlib.GameProfile;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.client.IRC;
import me.mioclient.module.player.NameProtect;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.client.util.SkinTextures;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({AbstractClientPlayerEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinAbstractClientPlayerEntity.class */
public abstract class MixinAbstractClientPlayerEntity extends PlayerEntity {
    private static Identifier PEPSI = Identifier.of("mio", "capes/pepsi.png");
    private static Identifier TETRIS = Identifier.of("mio", "capes/tetris.png");
    private static Identifier HEROBRINE = Identifier.of("mio-mount", "textures/skin_protect.png");
    private static final IRC irc = (IRC) BaritoneHelper_3.baritoneHelper_4.getModule117(IRC.class);
    private static final NameProtect nameprotect = (NameProtect) BaritoneHelper_3.baritoneHelper_4.getModule117(NameProtect.class);

    public MixinAbstractClientPlayerEntity(World world, BlockPos blockPos, float f, GameProfile gameProfile) {
        super(world, blockPos, f, gameProfile);
    }

    @Inject(method = {"getSkinTextures"}, at = {@At("RETURN")}, cancellable = true)
    private void getCape(CallbackInfoReturnable<SkinTextures> callbackInfoReturnable) {
        Identifier identifier2309;
        SkinTextures skinTextures = (SkinTextures) callbackInfoReturnable.getReturnValue();
        Identifier texture = skinTextures.texture();
        SkinTextures.Model model = skinTextures.model();
        if (nameprotect.isToggled() && nameprotect.skin.getValue().booleanValue() && MinecraftClient.getInstance().player.equals(this)) {
            texture = HEROBRINE;
            model = SkinTextures.Model.WIDE;
            if (nameprotect.slim.getValue().booleanValue()) {
                model = SkinTextures.Model.SLIM;
            }
            callbackInfoReturnable.setReturnValue(new SkinTextures(texture, skinTextures.textureUrl(), skinTextures.capeTexture(), skinTextures.elytraTexture(), model, skinTextures.secure()));
        }
        if ("cattyyyn".equals(getGameProfile().getName())) {
            callbackInfoReturnable.setReturnValue(new SkinTextures(texture, skinTextures.textureUrl(), PEPSI, skinTextures.elytraTexture(), model, skinTextures.secure()));
        }
        if ("u3o".equals(getGameProfile().getName())) {
            callbackInfoReturnable.setReturnValue(new SkinTextures(texture, skinTextures.textureUrl(), TETRIS, skinTextures.elytraTexture(), model, skinTextures.secure()));
        }
        if (BaritoneHelper_3.nameTagsSearchHelper4.is2305() && irc.capes.getValue().booleanValue() && (identifier2309 = BaritoneHelper_3.nameTagsSearchHelper4.getIdentifier2309(getGameProfile().getName())) != null) {
            callbackInfoReturnable.setReturnValue(new SkinTextures(texture, skinTextures.textureUrl(), identifier2309, skinTextures.elytraTexture(), model, skinTextures.secure()));
            callbackInfoReturnable.cancel();
        }
    }
}
