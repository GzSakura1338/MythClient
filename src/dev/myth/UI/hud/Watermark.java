package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.setting.Setting;
import net.fabricmc.loader.api.FabricLoader;

/** Mio Watermark, with Myth's name and installed version. */
public final class Watermark extends TextHudElement {
    public final Setting<Boolean> versionColor = addSetting(Setting.bool("versionColor", "VersionColor", true));
    public final Setting<String> text = addSetting(Setting.text("text", "Text", "Myth"));
    private final String version = FabricLoader.getInstance().getModContainer(Myth.MOD_ID)
            .orElseThrow().getMetadata().getVersion().getFriendlyString();

    public Watermark() { super("watermark", "Watermark"); setEnabled(true); }
    @Override public String getDescription() { return "Draws the client name and version."; }
    @Override protected String getText() {
        // ArgumentTypeHelper inserts in reverse order: text, optional white, version.
        return text.get().trim() + (versionColor.get() ? "\u00a7f" : "") + " v" + version;
    }
}
