package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;

public final class PlayerModel extends HudElement {
    public final Setting<Float> size = addSetting(Setting.decimal("size", "Size", 1, .5f, 2));
    public PlayerModel() { super("player_model", "PlayerModel"); }
    @Override public String getDescription() { return "Shows your live player model, skin and equipped items."; }
    @Override protected float measureWidth(FontManager fonts) { return 50 * size.get(); }
    @Override protected float measureHeight(FontManager fonts) { return 80 * size.get(); }
    @Override protected void render(RenderContext context) { Myth.HUD_MANAGER.assets.player(context.draw(), context.x(), context.y(), size.get(), context.opacity()); }
}
