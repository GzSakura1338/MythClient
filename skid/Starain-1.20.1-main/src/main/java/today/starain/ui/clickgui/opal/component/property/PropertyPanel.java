package today.starain.ui.clickgui.opal.component.property;

import today.starain.ui.clickgui.opal.component.OpalComponent;
import today.starain.value.Value;

public abstract class PropertyPanel extends OpalComponent {

    protected static final int DEFAULT_HEIGHT = 17;

    protected final Value value;
    protected boolean lastProperty;

    public PropertyPanel(Value value) {
        this.value = value;
        setHeight(DEFAULT_HEIGHT);
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        // The property-block background is drawn once by PropertyProvider so that adjacent
        // semi-transparent rows never leave anti-aliased seams ("white lines") between them.
        // Subclasses call super.render() then draw their own content on top.
    }

    public void setLastProperty(boolean lastProperty) {
        this.lastProperty = lastProperty;
    }

    public boolean isHidden() {
        return !value.isVisible();
    }
}
