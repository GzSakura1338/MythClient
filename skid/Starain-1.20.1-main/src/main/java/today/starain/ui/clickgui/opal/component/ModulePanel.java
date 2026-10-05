package today.starain.ui.clickgui.opal.component;

import lombok.Getter;
import lombok.Setter;
import today.starain.module.ClientModule;
import today.starain.ui.clickgui.opal.OpalClickGUI;
import today.starain.ui.clickgui.opal.OpalTheme;
import today.starain.ui.clickgui.opal.component.property.PropertyProvider;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.render.BindUtils;
import today.starain.utils.render.skia.Skia;

public final class ModulePanel extends OpalComponent {

    private static final String EXPAND_ICON = "";

    @Getter
    private final ClientModule module;
    @Getter
    private final Animation expandAnimation = new Animation(Easing.DECELERATE, OpalTheme.animExpand());
    @Getter
    private final Animation searchVisible = new Animation(Easing.DECELERATE, 220);
    private final PropertyProvider propertyProvider;
    private Animation hoverAnimation, toggleAnimation;
    @Getter
    private boolean lastModule;
    @Setter
    private int moduleIndex;
    private boolean expanded, selectingBind;

    public ModulePanel(ClientModule module) {
        this.module = module;
        this.propertyProvider = new PropertyProvider(module, this::isExpandedAnimation, this::isLastModule);
        this.searchVisible.setValue(1f);
    }

    private boolean isExpandedAnimation() {
        return expanded || expandAnimation.getValue() > 0F;
    }

    public boolean isExpanded() {
        return expanded;
    }

    public float getAddedHeight() {
        return propertyProvider.getExtraHeight();
    }

    public void setLastModule(boolean lastModule) {
        this.lastModule = lastModule;
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        handleAnimations(mouseX, mouseY);
        expandAnimation.setDuration(OpalTheme.animExpand());

        drawBackground(toggleAnimation.getValue(), hoverAnimation.getValue());

        OpalDraw.scissor(x, y, width, height, () -> {
            int color = module.isEnabled() ? OpalTheme.TEXT : OpalTheme.TEXT_MUTED;

            OpalDraw.text(module.getName(), x + 6, y + 12.5F, OpalTheme.FONT_ROW, color, module.isEnabled());

            if (propertyProvider.isHasProperties() && !selectingBind && !OpalClickGUI.displayingBinds) {
                float iconSize = OpalTheme.ICON_EXPAND;
                float pivotX = x + width - 17 + OpalDraw.iconWidth(EXPAND_ICON, iconSize) / 2f;
                float pivotY = y + 4 + iconSize / 2f;
                OpalDraw.rotate(pivotX, pivotY, expandAnimation.getValue() * 180,
                        () -> OpalDraw.centeredIcon(EXPAND_ICON, pivotX, pivotY, iconSize, color));
            }

            String keyString = null;
            if (selectingBind) {
                keyString = "[...]";
            } else if (OpalClickGUI.displayingBinds && module.getKey() != 0) {
                keyString = "[" + BindUtils.getBindName(module.getKey()) + "]";
            }

            if (keyString != null) {
                float textWidth = OpalDraw.textWidth(keyString, OpalTheme.FONT_BIND, false);
                OpalDraw.text(keyString, x + width - textWidth - 5, y + 12, OpalTheme.FONT_BIND, OpalTheme.TEXT, false);
            }

            if (expandAnimation.isFinished() && !expanded) return;

            propertyProvider.setX(x);
            propertyProvider.setY(y + OpalTheme.ROW_HEIGHT);
            propertyProvider.setWidth(width);
            propertyProvider.render(mouseX, mouseY, delta);
        });
    }

    /**
     * Draws the row background: frosted blur, base surface, accent gradient (when enabled)
     * and a hover overlay. The last module in a column gets rounded bottom corners; the hover
     * overlay is only shown while collapsed so it never bleeds onto expanded property rows.
     */
    private void drawBackground(float toggle, float hover) {
        int base = OpalDraw.opacity(OpalTheme.ROW_BG, OpalTheme.rowBgOpacity());
        // Mode-aware accent flowing diagonally from top-left toward bottom-left (Solid / Gradient / Rainbow).
        double phaseTop = -(x + y) * OpalTheme.accentRate();
        double phaseBottom = -(x + y + height) * OpalTheme.accentRate();
        int accent1 = OpalDraw.opacity(OpalTheme.accent(phaseTop), toggle);
        int accent2 = OpalDraw.opacity(OpalTheme.accent(phaseBottom), toggle);
        int hoverOverlay = OpalDraw.opacity(OpalTheme.TEXT, hover);
        boolean showHover = !expanded && hover > 0.001F;
        float r = OpalTheme.radius();

        if (!lastModule) {
            Skia.drawGuiBlur(x, y, width, height);
            Skia.drawRect(x, y, width, height, base);
            OpalDraw.gradientRect(x, y, width, height, accent1, accent2, 90);
            if (showHover) {
                Skia.drawRect(x, y, width, height, hoverOverlay);
            }
        } else {
            Skia.drawGuiRoundedBlurVarying(x, y, width, height, 0, 0, r, r);
            Skia.drawRoundedRectVarying(x, y, width, height, 0, 0, r, r, OpalDraw.color(base));
            OpalDraw.gradientRoundedRectVarying(x, y, width, height, 0, 0, r, r, accent1, accent2, 90);
            if (showHover) {
                Skia.drawRoundedRectVarying(x, y, width, height, 0, 0, r, r, OpalDraw.color(hoverOverlay));
            }
        }
    }

    private void handleAnimations(float mouseX, float mouseY) {
        float headerHeight = height - (expanded ? propertyProvider.getExtraHeight() : 0);

        float hoverTarget = isHovering(x, y, width, headerHeight, mouseX, mouseY) ? OpalTheme.hoverOpacity() : 0;
        if (this.hoverAnimation == null) {
            this.hoverAnimation = new Animation(Easing.DECELERATE, OpalTheme.animHover());
            this.hoverAnimation.setValue(hoverTarget);
        } else {
            this.hoverAnimation.setDuration(OpalTheme.animHover());
            this.hoverAnimation.run(hoverTarget);
        }

        float toggleTarget = module.isEnabled() ? OpalTheme.toggleTint() : 0;
        if (this.toggleAnimation == null) {
            this.toggleAnimation = new Animation(Easing.DECELERATE, OpalTheme.animToggle());
            this.toggleAnimation.setValue(toggleTarget);
        } else {
            this.toggleAnimation.setDuration(OpalTheme.animToggle());
            this.toggleAnimation.run(toggleTarget);
        }
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        if (selectingBind) {
            module.setKey(BindUtils.mouseCode(button));
            selectingBind = false;
            return;
        }

        float headerHeight = height - (expanded ? propertyProvider.getExtraHeight() : 0);
        if (isHovering(x, y, width, headerHeight, mouseX, mouseY)) {
            switch (button) {
                case 0 -> module.toogle();
                case 1 -> {
                    if (propertyProvider.isHasProperties()) {
                        expanded = !expanded;
                    }
                }
                case 2 -> selectingBind = true;
            }
        }

        propertyProvider.mouseClicked(mouseX, mouseY, button);
    }

    @Override
    public void keyPressed(int key) {
        if (selectingBind) {
            module.setKey(key == 256 ? 0 : BindUtils.keyboardCode(key));
            selectingBind = false;
            return;
        }
        propertyProvider.keyPressed(key);
    }

    @Override
    public void charTyped(int codepoint) {
        propertyProvider.charTyped(codepoint);
    }

    @Override
    public void mouseReleased(double mouseX, double mouseY, int button) {
        propertyProvider.mouseReleased(mouseX, mouseY, button);
    }

    @Override
    public void mouseScrolled(double mouseX, double mouseY, double verticalAmount) {
        propertyProvider.mouseScrolled(mouseX, mouseY, verticalAmount);
    }

    @Override
    public void init() {
        selectingBind = false;
        propertyProvider.init();
    }

    @Override
    public void close() {
        propertyProvider.close();
    }
}
