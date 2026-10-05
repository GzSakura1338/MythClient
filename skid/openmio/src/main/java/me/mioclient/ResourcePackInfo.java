package me.mioclient;

import java.io.InputStream;
import java.util.Optional;
import java.util.Set;
import net.minecraft.resource.InputSupplier;
import net.minecraft.resource.ResourcePack;
import net.minecraft.resource.ResourcePackSource;
import net.minecraft.resource.ResourceType;
import net.minecraft.resource.metadata.ResourceMetadataReader;
import net.minecraft.text.Text;
import net.minecraft.util.Identifier;
import org.jetbrains.annotations.Nullable;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ResourcePackInfo.class */
public class ResourcePackInfo implements ResourcePack {
    public net.minecraft.resource.ResourcePackInfo info = new net.minecraft.resource.ResourcePackInfo("mio", Text.literal("mio"), ResourcePackSource.BUILTIN, Optional.empty());

    @Nullable
    public InputSupplier<InputStream> openRoot(String... strArr) {
        return null;
    }

    @Nullable
    public InputSupplier<InputStream> open(ResourceType resourceType, Identifier identifier) {
        return null;
    }

    public void findResources(ResourceType resourceType, String str, String str2, ResourcePack.ResultConsumer resultConsumer) {
    }

    public Set<String> getNamespaces(ResourceType resourceType) {
        return null;
    }

    @Nullable
    public <T> T parseMetadata(ResourceMetadataReader<T> resourceMetadataReader) {
        return null;
    }

    public net.minecraft.resource.ResourcePackInfo getInfo() {
        return this.info;
    }

    public void close() {
    }
}
