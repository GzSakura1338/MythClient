package me.mioclient.mixin;

import com.mojang.authlib.GameProfile;
import me.mioclient.AutoCrystalHelper_2;
import net.minecraft.client.network.PlayerListEntry;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({PlayerListEntry.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinPlayerListEntry.class */
public class MixinPlayerListEntry implements AutoCrystalHelper_2 {

    @Unique
    private long mio$joinTime;

    @Inject(method = {"<init>(Lcom/mojang/authlib/GameProfile;Z)V"}, at = {@At("TAIL")})
    private void mio$init(GameProfile gameProfile, boolean z, CallbackInfo callbackInfo) {
        this.mio$joinTime = System.currentTimeMillis();
    }

    @Override // me.mioclient.AutoCrystalHelper_2
    public long mio$getJoinTime() {
        return this.mio$joinTime;
    }
}
