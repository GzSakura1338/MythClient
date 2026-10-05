package today.starain.ui.clickgui.opal.component;

import net.minecraft.Util;
import net.minecraft.client.Minecraft;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.ui.clickgui.opal.OpalTheme;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.client.ChatUtils;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Icon;
import today.starain.utils.wrapper.Wrapper;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Opal-style Config category column. Mirrors {@link CategoryPanel}'s header and
 * open/close animation, but renders the saved config files as rows plus a compact
 * action row (new / save / delete / refresh). The header folder icon opens the folder.
 */
public final class ConfigCategoryPanel extends OpalComponent {

    private static final int BASE_COLOR = 0xff1e1e2d;

    private final Scroller scroller = new Scroller();
    private final int panelIndex;
    private final boolean lastPanel;

    private final List<String> configs = new ArrayList<>();
    private int selectedIndex = -1;
    private float renderScroll;

    private Animation openAnimation;
    private boolean closing;

    public ConfigCategoryPanel(int panelIndex, int panelCount) {
        this.panelIndex = panelIndex;
        this.lastPanel = panelIndex == panelCount - 1;
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        openAnimation.run(closing ? 0 : 1);

        if (lastPanel && openAnimation.isFinished() && closing) {
            Minecraft.getInstance().setScreen(null);
            return;
        }

        float totalHeight = getTotalHeight();
        float openValue = openAnimation.getValue();
        float scaledHeight = Minecraft.getInstance().getWindow().getGuiScaledHeight();
        float scissorHeight = Math.min(scaledHeight - y, totalHeight * openValue);
        float scrollOffset = scroller.getAnimation().getValue();
        this.renderScroll = scrollOffset;

        OpalDraw.alpha(openValue, () -> OpalDraw.scissor(x, y, width, scissorHeight, () -> {
            // Header
            Skia.drawGuiRoundedBlur(x, y + scrollOffset, width, height, OpalTheme.radius());
            Skia.drawRoundedRectVarying(x, y + scrollOffset, width, height, OpalTheme.radius(), OpalTheme.radius(), 0, 0,
                    OpalDraw.color(OpalDraw.opacity(OpalTheme.HEADER_BG, OpalTheme.headerBgOpacity())));
            OpalDraw.text(Category.CONFIG.getName(), x + 5, y + scrollOffset + 13, OpalTheme.FONT_HEADER, OpalTheme.TEXT, true);
            OpalDraw.icon(Category.CONFIG.getIcon(), x + width - 15.5F, y + scrollOffset + 15, OpalTheme.ICON_HEADER, OpalTheme.TEXT, false);

            // Config rows: one continuous frosted background (matches the module list, no seams)
            if (!configs.isEmpty()) {
                float listY = y + height + scrollOffset;
                float listH = configs.size() * height;
                Skia.drawGuiBlur(x, listY, width, listH);
                Skia.drawRect(x, listY, width, listH, OpalDraw.opacity(OpalTheme.ROW_BG, OpalTheme.rowBgOpacity()));
            }
            for (int i = 0; i < configs.size(); i++) {
                float rowY = y + height + i * height + scrollOffset;
                boolean sel = i == selectedIndex;
                boolean hov = isHovering(x, rowY, width, height, mouseX, mouseY);
                if (sel) {
                    OpalDraw.gradientRect(x, rowY, width, height,
                            OpalDraw.opacity(OpalTheme.accent(-(x + rowY) * OpalTheme.accentRate()), OpalTheme.toggleTint()),
                            OpalDraw.opacity(OpalTheme.accent(-(x + rowY + height) * OpalTheme.accentRate()), OpalTheme.toggleTint()), 90);
                } else if (hov) {
                    Skia.drawRect(x, rowY, width, height, OpalDraw.opacity(OpalTheme.TEXT, OpalTheme.hoverOpacity()));
                }
                int textColor = sel ? OpalTheme.TEXT : OpalTheme.TEXT_MUTED;
                OpalDraw.text(configs.get(i) + ".cfg", x + 6, rowY + 12.5F, OpalTheme.FONT_ROW, textColor, sel);
            }

            // Action row (rounded bottom)
            float actionY = y + height + configs.size() * height + scrollOffset;
            Skia.drawGuiRoundedBlur(x, actionY, width, height, OpalTheme.radius());
            Skia.drawRoundedRectVarying(x, actionY, width, height, 0, 0, OpalTheme.radius(), OpalTheme.radius(),
                    OpalDraw.color(OpalDraw.opacity(OpalTheme.ROW_BG, OpalTheme.rowBgOpacity())));
            String[] icons = {Icon.ADD, Icon.SAVE, Icon.FILE_DOWNLOAD, Icon.DELETE, Icon.FOLDER_OPEN, Icon.REFRESH};
            float slotW = width / icons.length;
            for (int k = 0; k < icons.length; k++) {
                float cx = x + slotW * (k + 0.5F);
                float cy = actionY + height / 2F;
                boolean hov = isHovering(x + slotW * k, actionY, slotW, height, mouseX, mouseY);
                OpalDraw.centeredIcon(icons[k], cx, cy, 11, hov ? OpalTheme.first() : OpalTheme.TEXT_MUTED);
            }
        }));

        scroller.onScroll(getMaxOffset(totalHeight));
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        float scroll = renderScroll;

        // Folder icon (header, right side)
        if (isHovering(x + width - 24, y + scroll, 24, height, mouseX, mouseY)) {
            openFolder();
            return;
        }

        // Config rows
        for (int i = 0; i < configs.size(); i++) {
            float rowY = y + height + i * height + scroll;
            if (isHovering(x, rowY, width, height, mouseX, mouseY)) {
                // Left-click selects; loading is done via the Load action button.
                selectedIndex = i;
                return;
            }
        }

        // Action row
        float actionY = y + height + configs.size() * height + scroll;
        if (isHovering(x, actionY, width, height, mouseX, mouseY)) {
            int slot = (int) ((mouseX - x) / (width / 6F));
            switch (slot) {
                case 0 -> newConfig();
                case 1 -> saveSelected();
                case 2 -> loadSelected();
                case 3 -> deleteSelected();
                case 4 -> openFolder();
                case 5 -> {
                    refresh();
                    ChatUtils.addChatMessage("[Config] Refreshed");
                }
                default -> {
                }
            }
        }
    }

    @Override
    public void mouseScrolled(double mouseX, double mouseY, double verticalAmount) {
        float totalHeight = getTotalHeight();
        if (isHovering(x, y, width, totalHeight, mouseX, mouseY)) {
            scroller.addScroll(verticalAmount, getMaxOffset(totalHeight));
        }
    }

    @Override
    public void init() {
        this.openAnimation = new Animation(Easing.EASE_OUT_SINE, 100 + (panelIndex * 80L));
        this.closing = false;
        refresh();
    }

    @Override
    public void close() {
        this.closing = true;
    }

    private float getTotalHeight() {
        // header + config rows + action row
        return height * (configs.size() + 2);
    }

    private float getMaxOffset(float totalHeight) {
        float relativeScreenHeight = Minecraft.getInstance().getWindow().getGuiScaledHeight() - y;
        float scissorHeight = Math.min(relativeScreenHeight, totalHeight * openAnimation.getValue());
        float overflowPadding = scissorHeight == relativeScreenHeight ? y : 0;
        return Math.max(0, totalHeight - scissorHeight + overflowPadding);
    }

    private void refresh() {
        configs.clear();
        File dir = Wrapper.resolveConfigFolder();
        if (!dir.exists()) {
            dir.mkdirs();
        }
        File[] files = dir.listFiles((d, n) -> n.toLowerCase().endsWith(".json"));
        if (files != null) {
            List<String> names = new ArrayList<>();
            for (File f : files) {
                String n = f.getName();
                names.add(n.substring(0, n.length() - 5));
            }
            names.sort(String.CASE_INSENSITIVE_ORDER);
            configs.addAll(names);
        }
        selectedIndex = configs.indexOf(Starain.getInstance().getConfigManager().getCurrentConfig());
    }

    private String selectedName() {
        if (selectedIndex < 0 || selectedIndex >= configs.size()) {
            return null;
        }
        return configs.get(selectedIndex);
    }

    private void newConfig() {
        String name = "config";
        int i = 1;
        while (new File(Wrapper.resolveConfigFolder(), name + ".json").exists()) {
            name = "config-" + i++;
        }
        Starain.getInstance().getConfigManager().saveConfig(name);
        refresh();
        selectedIndex = configs.indexOf(name);
        ChatUtils.addChatMessage("[Config] Created ", name);
    }

    private void saveSelected() {
        String name = selectedName();
        if (name == null) {
            ChatUtils.addChatMessage("[Config] No config selected");
            return;
        }
        Starain.getInstance().getConfigManager().saveConfig(name);
        ChatUtils.addChatMessage("[Config] Saved ", name);
    }

    private void loadSelected() {
        String name = selectedName();
        if (name == null) {
            ChatUtils.addChatMessage("[Config] No config selected");
            return;
        }
        Starain.getInstance().getConfigManager().loadConfig(name);
        ChatUtils.addChatMessage("[Config] Loaded ", name);
    }

    private void deleteSelected() {
        String name = selectedName();
        if (name == null) {
            ChatUtils.addChatMessage("[Config] No config selected");
            return;
        }
        File f = new File(Wrapper.resolveConfigFolder(), name + ".json");
        if (f.delete()) {
            ChatUtils.addChatMessage("[Config] Deleted ", name);
        } else {
            ChatUtils.addChatMessage("[Config] Failed to delete ", name);
        }
        refresh();
    }

    private void openFolder() {
        try {
            File dir = Wrapper.resolveConfigFolder();
            if (!dir.exists()) {
                dir.mkdirs();
            }
            Util.getPlatform().openFile(dir);
        } catch (Exception e) {
            ChatUtils.addChatMessage("[Config] Failed to open folder: ", String.valueOf(e.getMessage()));
        }
    }
}
