package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyReturnValue;
import com.llamalad7.mixinextras.sugar.Local;
import java.io.FileInputStream;
import java.util.Optional;
import me.mioclient.Helper_16;
import me.mioclient.PresetHelper;
import me.mioclient.ResourcePackInfo;
import me.mioclient.SearchHelper_4;
import net.minecraft.resource.ReloadableResourceManagerImpl;
import net.minecraft.resource.Resource;
import net.minecraft.util.Identifier;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

/* compiled from: 0.java */
@Mixin({ReloadableResourceManagerImpl.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinReloadableResourceManagerImpl.class */
public class MixinReloadableResourceManagerImpl {
    @ModifyReturnValue(method = {"getResource"}, at = {@At("RETURN")})
    private Optional<Resource> getResource(Optional<Resource> optional, @Local(argsOnly = true) Identifier identifier) {
        if (identifier.getNamespace().equals("mio-mount")) {
            return Optional.of(new Resource(new ResourcePackInfo(), () -> {
                return new FileInputStream(PresetHelper.path.resolve(identifier.getPath()).toFile());
            }));
        }
        if (!optional.isEmpty() || ((!identifier.getNamespace().equals("mio") && !identifier.getPath().contains("/blur_mask.")) || !SearchHelper_4.is1471())) {
            return optional;
        }
        String str = identifier.getPath().contains("blur_mask") ? "minecraft" : "mio";
        return Optional.of(new Resource(new ResourcePackInfo(), () -> {
            return Helper_16.class.getClassLoader().getResourceAsStream("assets/%s/%s".formatted(str, identifier.getPath()));
        }));
    }
}
