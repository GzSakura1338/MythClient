package today.starain.ui.clickgui.opal.component.property;

import today.starain.ui.clickgui.opal.OpalTheme;
import today.starain.ui.clickgui.opal.component.ToggleSwitch;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.value.impl.BooleanValue;

public final class BooleanPanel extends PropertyPanel {

    private final BooleanValue booleanValue;
    private final ToggleSwitch toggleSwitch;

    public BooleanPanel(BooleanValue value) {
        super(value);
        this.booleanValue = value;
        this.toggleSwitch = new ToggleSwitch(value::toggle, value::get);
    }

    @Override
    public void init() {
        toggleSwitch.reset();
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        super.render(mouseX, mouseY, delta);

        OpalDraw.text(booleanValue.getName(), x + 5, y + 10.5F, 7, 0xffffffff, false);

        toggleSwitch.setBoxColors(OpalTheme.first(), 0xff3c3c3c);
        toggleSwitch.render(x + width - 22, y + 3.8F, 0.85F);
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        if (button == 0) {
            toggleSwitch.mouseClicked(x, y, width, height, mouseX, mouseY);
        }
    }
}
