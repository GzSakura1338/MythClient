package dev.myth.UI;

import dev.myth.Myth;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.client.Minecraft;
import net.minecraft.client.resources.sounds.SimpleSoundInstance;
import net.minecraft.resources.Identifier;
import net.minecraft.sounds.SoundEvent;

final class UiSounds {
    record Entry(String group, String name, String key) {}
    static final List<String> NAMES = List.of("kaban", "neverlose", "css", "cod", "quake", "toolbox", "warning",
            "steam", "whatsapp", "vk", "icq", "stalker", "hover", "click", "rclick", "bodysplat");
    private static final Map<String, byte[]> custom = new LinkedHashMap<>();
    private static Path directory() { return Minecraft.getInstance().gameDirectory.toPath().resolve("config/myth/sounds"); }
    static List<Entry> reload() {
        List<Entry> entries = new ArrayList<>();
        NAMES.forEach(name -> entries.add(new Entry("System", name, name)));
        custom.clear();
        Path root = directory();
        if (!Files.isDirectory(root)) return entries;
        try (var folders = Files.list(root)) {
            for (Path folder : folders.filter(Files::isDirectory).sorted().toList()) {
                try (var files = Files.list(folder)) {
                    for (Path file : files.filter(path -> Files.isRegularFile(path) && path.getFileName().toString().endsWith(".ogg")).sorted().toList()) {
                        String group = folder.getFileName().toString();
                        String name = file.getFileName().toString().replaceFirst("\\.ogg$", "");
                        String key = group + "/" + name;
                        custom.put(key, Files.readAllBytes(file)); entries.add(new Entry(group, name, key));
                    }
                }
            }
        } catch (IOException exception) { Myth.LOGGER.error("Could not load custom UI sounds", exception); }
        return entries;
    }
    static void importFiles(List<Path> files) {
        Path destination = directory().resolve("custom");
        for (Path file : files) if (Files.isRegularFile(file) && file.getFileName().toString().endsWith(".ogg")) {
            try { Files.createDirectories(destination); Files.copy(file, destination.resolve(file.getFileName())); }
            catch (IOException exception) { Myth.LOGGER.error("Could not import UI sound {}", file.getFileName(), exception); }
        }
    }
    static void play(String name, float volume) {
        if (!NAMES.contains(name)) {
            if (!custom.containsKey(name)) reload();
            byte[] bytes = custom.get(name);
            if (bytes != null) Minecraft.getInstance().getSoundManager().play(new InputStreamSoundInstance(new ByteArrayInputStream(bytes), volume));
            return;
        }
        var sound = SoundEvent.createVariableRangeEvent(Identifier.fromNamespaceAndPath("myth", "ui." + name));
        Minecraft.getInstance().getSoundManager().play(SimpleSoundInstance.forUI(sound, 1f, volume));
    }
}
