package dev.myth.UI;

import java.io.InputStream;
import java.util.concurrent.CompletableFuture;
import net.fabricmc.fabric.api.client.sound.v1.FabricSoundInstance;
import net.minecraft.client.resources.sounds.AbstractSoundInstance;
import net.minecraft.client.sounds.AudioStream;
import net.minecraft.client.sounds.JOrbisAudioStream;
import net.minecraft.client.resources.sounds.SoundInstance;
import net.minecraft.client.sounds.SoundBufferLibrary;
import net.minecraft.sounds.SoundSource;
import net.minecraft.resources.Identifier;
import net.minecraft.world.phys.Vec3;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/InputStreamAbstractSoundInstance.class */
public class InputStreamSoundInstance extends AbstractSoundInstance implements FabricSoundInstance {
    public InputStream inputStream;

    public InputStreamSoundInstance(Vec3 vec3d, InputStream inputStream) {
        super(Identifier.fromNamespaceAndPath("myth", "ui.custom"), SoundSource.MASTER, SoundInstance.createUnseededRandom());
        this.x = vec3d.x;
        this.y = vec3d.y;
        this.z = vec3d.z;
        this.inputStream = inputStream;
    }

    public InputStreamSoundInstance(InputStream inputStream, float volume) {
        super(Identifier.fromNamespaceAndPath("myth", "ui.custom"), SoundSource.MASTER, SoundInstance.createUnseededRandom());
        this.inputStream = inputStream;
        this.volume = volume;
        this.attenuation = SoundInstance.Attenuation.NONE;
        this.relative = true;
        this.x = 0.0d;
        this.y = 0.0d;
        this.z = Double.longBitsToDouble(-4616189618054758400L);
    }

    @Override
    public CompletableFuture<AudioStream> getAudioStream(SoundBufferLibrary soundLoader, Identifier identifier, boolean z) {
        try {
            return CompletableFuture.completedFuture(new JOrbisAudioStream(this.inputStream));
        } catch (Exception e) {
            return CompletableFuture.failedFuture(e);
        }
    }
}
