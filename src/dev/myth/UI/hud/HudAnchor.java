package dev.myth.UI.hud;

/** Openmio ModuleListMode docking groups. */
public enum HudAnchor {
    TOP_LEFT("top_left"), TOP_RIGHT("top_right"), BOTTOM_LEFT("bottom_left"),
    BOTTOM_RIGHT("bottom_right"), TOP_CENTER("top_center"), NONE("none");

    private final String id;
    HudAnchor(String id) { this.id = id; }
    public String getId() { return id; }
    public boolean isBottom() { return this == BOTTOM_LEFT || this == BOTTOM_RIGHT; }
    public static HudAnchor parse(String value) {
        for (HudAnchor anchor : values()) if (anchor.id.equalsIgnoreCase(value) || anchor.name().equalsIgnoreCase(value)) return anchor;
        throw new IllegalArgumentException("Unknown HUD anchor: " + value);
    }
}
