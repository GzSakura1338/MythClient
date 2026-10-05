package dev.myth.config;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

/** UTF-8 profiles with atomic replacement. Does not depend on the game runtime. */
public final class Config {
    private static final Gson GSON = new GsonBuilder().setPrettyPrinting().create();
    private final String name;
    private final Path file;

    public Config(Path directory, String name) {
        this.name = normalizeName(name);
        this.file = directory.toAbsolutePath().normalize().resolve(this.name + ".json");
    }

    public static String normalizeName(String name) {
        if (name == null) {
            throw new IllegalArgumentException("Config name must not be empty");
        }
        String normalized = name.trim();
        if (!normalized.matches("[\\p{L}\\p{N}_-][\\p{L}\\p{N}_.-]{0,63}")
                || normalized.endsWith(".")
                || normalized.matches("(?i)(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\\..*)?")) {
            throw new IllegalArgumentException("Invalid config name. Use letters, numbers, '_', '-' or '.', up to 64 characters.");
        }
        return normalized;
    }

    public String getName() {
        return name;
    }

    public Path getFile() {
        return file;
    }

    public boolean exists() {
        return Files.isRegularFile(file);
    }

    public JsonObject load() throws IOException {
        try {
            var parsed = JsonParser.parseString(Files.readString(file, StandardCharsets.UTF_8));
            if (!parsed.isJsonObject()) {
                throw new IllegalArgumentException("Config must be a JSON object");
            }
            JsonObject root = parsed.getAsJsonObject();
            if (!root.has("modules") || !root.get("modules").isJsonObject()) {
                throw new IllegalArgumentException("Config is missing the modules object");
            }
            if (root.has("version") && root.get("version").getAsInt() != 1) {
                throw new IllegalArgumentException("Unsupported config version");
            }
            return root;
        } catch (RuntimeException exception) {
            throw new IOException("Invalid config format: " + name, exception);
        }
    }

    public void save(JsonObject root) throws IOException {
        Files.createDirectories(file.getParent());
        Path temporary = Files.createTempFile(file.getParent(), ".myth-", ".tmp");
        try {
            Files.writeString(temporary, GSON.toJson(root) + System.lineSeparator(), StandardCharsets.UTF_8);
            try {
                Files.move(temporary, file, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
            } catch (AtomicMoveNotSupportedException exception) {
                Files.move(temporary, file, StandardCopyOption.REPLACE_EXISTING);
            }
        } finally {
            Files.deleteIfExists(temporary);
        }
    }
}
