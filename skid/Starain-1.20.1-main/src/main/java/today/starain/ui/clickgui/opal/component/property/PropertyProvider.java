package today.starain.ui.clickgui.opal.component.property;

import lombok.Getter;
import today.starain.module.ClientModule;
import today.starain.ui.clickgui.opal.OpalTheme;
import today.starain.ui.clickgui.opal.component.OpalComponent;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.render.skia.Skia;
import today.starain.value.Value;
import today.starain.value.impl.*;

import java.util.ArrayList;
import java.util.List;
import java.util.function.BooleanSupplier;

public final class PropertyProvider extends OpalComponent {

    private static final int PROPERTY_BG = 0xff000000;
    private static final float PROPERTY_BG_OPACITY = 0.25F;

    private final List<PropertyPanel> propertyPanels = new ArrayList<>();
    private final ClientModule module;
    private final BooleanSupplier expanded;
    private final BooleanSupplier lastModule;

    @Getter
    private float extraHeight;

    public PropertyProvider(ClientModule module, BooleanSupplier expanded, BooleanSupplier lastModule) {
        this.module = module;
        this.expanded = expanded;
        this.lastModule = lastModule;
        for (Value value : module.getValues()) {
            PropertyPanel panel = create(value);
            if (panel != null) {
                propertyPanels.add(panel);
            }
        }
    }

    public static PropertyPanel create(Value value) {
        if (value instanceof BooleanValue booleanValue) return new BooleanPanel(booleanValue);
        if (value instanceof NumberValue numberValue) {
            return numberValue.isRange() ? new RangeNumberPanel(numberValue) : new NumberPanel(numberValue);
        }
        if (value instanceof ModeValue modeValue) return new ModePanel(modeValue);
        if (value instanceof ColorValue colorValue) return new ColorPanel(colorValue);
        if (value instanceof TextValue textValue) return new StringPanel(textValue);
        return null;
    }

    public boolean isHasProperties() {
        return module.getValues().stream().anyMatch(Value::isVisible);
    }

    private boolean isClosed() {
        return !expanded.getAsBoolean();
    }

    @Override
    public void init() {
        if (isClosed()) return;
        propertyPanels.forEach(PropertyPanel::init);
    }

    @Override
    public void close() {
        if (isClosed()) return;
        propertyPanels.forEach(PropertyPanel::close);
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        if (isClosed()) return;

        float total = 0f;
        int lastVisibleIndex = -1;
        for (int i = 0; i < propertyPanels.size(); i++) {
            PropertyPanel panel = propertyPanels.get(i);
            if (panel.isHidden()) continue;
            total += panel.getHeight();
            lastVisibleIndex = i;
        }

        // Single continuous background for the whole property block. Drawing it once (instead
        // of per row) avoids anti-aliased seams between adjacent semi-transparent rows.
        if (total > 0f) {
            int bg = OpalDraw.opacity(PROPERTY_BG, PROPERTY_BG_OPACITY);
            boolean roundBottom = lastModule != null && lastModule.getAsBoolean();
            if (roundBottom) {
                Skia.drawRoundedRectVarying(x, y, width, total, 0, 0, OpalTheme.radius(), OpalTheme.radius(), OpalDraw.color(bg));
            } else {
                Skia.drawRect(x, y, width, total, bg);
            }
        }

        float offset = 0;
        for (int i = 0; i < propertyPanels.size(); i++) {
            PropertyPanel panel = propertyPanels.get(i);
            if (panel.isHidden()) continue;

            panel.setX(x);
            panel.setY(y + offset);
            panel.setWidth(width);
            panel.setLastProperty(lastModule != null && lastModule.getAsBoolean() && i == lastVisibleIndex);

            panel.render(mouseX, mouseY, delta);

            offset += panel.getHeight();
        }

        this.extraHeight = offset;
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        if (isClosed()) return;
        for (PropertyPanel panel : propertyPanels) {
            if (!panel.isHidden()) panel.mouseClicked(mouseX, mouseY, button);
        }
    }

    @Override
    public void mouseReleased(double mouseX, double mouseY, int button) {
        if (isClosed()) return;
        for (PropertyPanel panel : propertyPanels) {
            if (!panel.isHidden()) panel.mouseReleased(mouseX, mouseY, button);
        }
    }

    @Override
    public void mouseScrolled(double mouseX, double mouseY, double verticalAmount) {
        if (isClosed()) return;
        for (PropertyPanel panel : propertyPanels) {
            if (!panel.isHidden()) panel.mouseScrolled(mouseX, mouseY, verticalAmount);
        }
    }

    @Override
    public void keyPressed(int key) {
        if (isClosed()) return;
        propertyPanels.forEach(panel -> panel.keyPressed(key));
    }

    @Override
    public void charTyped(int codepoint) {
        if (isClosed()) return;
        propertyPanels.forEach(panel -> panel.charTyped(codepoint));
    }
}
