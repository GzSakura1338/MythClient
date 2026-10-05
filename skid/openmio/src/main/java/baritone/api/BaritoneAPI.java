package baritone.api;
/** Minimal stub of the Baritone API surface used by this mod (Baritone is not bundled). */
public final class BaritoneAPI {
    private static final IBaritoneProvider PROVIDER = null;
    private static final Settings SETTINGS = new Settings();
    public static IBaritoneProvider getProvider() { return PROVIDER; }
    public static Settings getSettings() { return SETTINGS; }
}
