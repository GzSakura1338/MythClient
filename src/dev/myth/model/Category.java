package dev.myth.model;

public enum Category {
    COMBAT("combat"),
    MISC("misc"),
    PLAYER("player"),
    RENDER("render"),
    CLIENT("client");

    private final String id;

    Category(String id) {
        this.id = id;
    }

    public String getId() {
        return id;
    }
}
