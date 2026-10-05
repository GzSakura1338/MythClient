package dev.myth.UI.hud;

import com.google.gson.JsonParser;
import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

/** Mio Music presentation backed by Windows' active media session. */
public final class Music extends HudElement {
    private volatile String track;
    private volatile boolean pending;
    private volatile Process process;
    private long nextPoll;
    public Music() { super("music", "Music"); }
    @Override public String getDescription() { return "Shows the active Windows media session's artist and title."; }
    private String display() { return track == null && Myth.UI_MANAGER.isHudEditorOpen() ? "HydrachFM - f3dot" : track; }
    @Override protected void onEnable() { if (!System.getProperty("os.name").startsWith("Windows")) setEnabled(false); }
    @Override protected void onTick() {
        if (pending || System.currentTimeMillis() < nextPoll) return;
        nextPoll = System.currentTimeMillis() + 2000; pending = true;
        CompletableFuture.runAsync(() -> {
            try (var source = Music.class.getResourceAsStream("/assets/myth/hud/media-session.ps1")) {
                if (source == null) return;
                String script = new String(source.readAllBytes(), StandardCharsets.UTF_8);
                String encoded = java.util.Base64.getEncoder().encodeToString(script.getBytes(StandardCharsets.UTF_16LE));
                process = new ProcessBuilder("powershell.exe", "-NoProfile", "-NonInteractive", "-WindowStyle", "Hidden", "-EncodedCommand", encoded)
                        .redirectError(ProcessBuilder.Redirect.DISCARD).start();
                if (!process.waitFor(6, TimeUnit.SECONDS)) { process.destroyForcibly(); track = null; return; }
                String output = new String(process.getInputStream().readAllBytes(), StandardCharsets.UTF_8).trim();
                if (output.isEmpty()) { track = null; return; }
                var data = JsonParser.parseString(output).getAsJsonObject();
                track = data.get("playing").getAsBoolean() ? data.get("artist").getAsString() + " - " + data.get("title").getAsString() : null;
            } catch (Exception ignored) { track = null; }
            finally { process = null; pending = false; }
        });
    }
    @Override protected float measureWidth(FontManager fonts) { String text = display(); return text == null ? 0 : fonts.width(text); }
    @Override protected float measureHeight(FontManager fonts) { return display() == null ? 0 : fonts.height(); }
    @Override protected void render(RenderContext context) {
        String text = display();
        if (text != null) context.fonts().draw(context.draw(), text, context.x(), context.y(), Myth.HUD_MANAGER.colors.colorAt(context.y(), context.fonts().height()), context.opacity());
    }
    @Override public void close() { if (process != null) process.destroyForcibly(); }
}
