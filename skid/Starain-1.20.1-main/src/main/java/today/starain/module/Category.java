package today.starain.module;

import lombok.Getter;
import today.starain.utils.render.skia.font.Icon;

@Getter
public enum Category {
    COMBAT("Combat", Icon.SWORDS),
    PLAYER("Player", Icon.PERSON),
    MOVEMENT("Movement", Icon.DIRECTIONS_RUN),
    RENDER("Visual", Icon.VISIBILITY),
    MISC("Misc", Icon.SETTINGS),
    CLIENT("Client", Icon.AUTO_AWESOME),
    CONFIG("Config", Icon.FOLDER);

    private final String name;
    private final String icon;

    Category(String name, String icon) {
        this.name = name;
        this.icon = icon;
    }
}
