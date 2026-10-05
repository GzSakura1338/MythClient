package today.starain.module.impl.client;

import lombok.Getter;
import lombok.Setter;
import org.lwjgl.glfw.GLFW;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.ui.clickgui.opal.OpalClickGUI;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ColorValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;

import java.awt.*;

public final class ClickGuiModule extends ClientModule {
    @Setter
    @Getter
    private static volatile ClientModule bindingTarget;

    private final ModeValue mode = new ModeValue("Mode", "Opal", new String[]{"Opal"}, this);

    private final NumberValue radius = new NumberValue("Radius", 0, 12, 5, 0.5, this);
    private final NumberValue panelWidth = new NumberValue("Panel Width", 80, 160, 110, 1, this);
    private final NumberValue bgOpacity = new NumberValue("Background Opacity", 0, 100, 70, 1, this);
    private final NumberValue hoverIntensity = new NumberValue("Hover Intensity", 0, 100, 10, 1, this);
    private final NumberValue accentIntensity = new NumberValue("Accent Intensity", 0, 100, 40, 1, this);

    private final BooleanValue animations = new BooleanValue("Animations", true, this);
    private final NumberValue animSpeed = new NumberValue("Animation Speed", 0.25, 3.0, 1.0, 0.05, this, animations::get);

    private final BooleanValue syncColor = new BooleanValue("Sync Color", true, this);
    private final ColorValue accentColor = new ColorValue("Accent Color", new Color(91, 207, 250), this, () -> !syncColor.get());
    private final BooleanValue customFlow = new BooleanValue("Custom Flow", false, this, syncColor::get);
    private final NumberValue flowSpeed = new NumberValue("Flow Speed", 1, 50, 25, 1, this, () -> syncColor.get() && customFlow.get());
    private final NumberValue flowDensity = new NumberValue("Flow Density", 1, 100, 10, 1, this, () -> syncColor.get() && customFlow.get());

    private final NumberValue guiX = new NumberValue("GUI X", -5000.0, 10000.0, 50.0, 1.0, this, () -> false);
    private final NumberValue guiY = new NumberValue("GUI Y", -5000.0, 10000.0, 50.0, 1.0, this, () -> false);

    public ClickGuiModule() {
        setName("ClickGui");
        setCategory(Category.CLIENT);
        setKey(GLFW.GLFW_KEY_RIGHT_SHIFT);
    }

    private static ClickGuiModule get() {
        Starain instance = Starain.getInstance();
        return instance == null ? null : instance.getModuleManager().getModule(ClickGuiModule.class);
    }

    public static boolean isBindingMode() {
        return bindingTarget != null;
    }

    public static float getGuiX() {
        ClickGuiModule m = get();
        return m != null ? m.guiX.get().floatValue() : 50.0F;
    }

    public static float getGuiY() {
        ClickGuiModule m = get();
        return m != null ? m.guiY.get().floatValue() : 50.0F;
    }

    public static void saveGuiPos(float x, float y) {
        ClickGuiModule m = get();
        if (m != null) {
            m.guiX.setValue(x);
            m.guiY.setValue(y);
        }
    }

    public static float guiRadius() {
        ClickGuiModule m = get();
        return m != null ? m.radius.get().floatValue() : 5.0F;
    }

    public static float guiPanelWidth() {
        ClickGuiModule m = get();
        return m != null ? m.panelWidth.get().floatValue() : 110.0F;
    }

    public static float guiRowOpacity() {
        ClickGuiModule m = get();
        return m != null ? m.bgOpacity.get().floatValue() / 100.0F : 0.7F;
    }

    public static float guiHeaderOpacity() {
        ClickGuiModule m = get();
        return m != null ? m.bgOpacity.get().floatValue() / 100.0F : 0.7F;
    }

    public static float guiHoverOpacity() {
        ClickGuiModule m = get();
        return m != null ? m.hoverIntensity.get().floatValue() / 100.0F : 0.1F;
    }

    public static float guiAccentTint() {
        ClickGuiModule m = get();
        return m != null ? m.accentIntensity.get().floatValue() / 100.0F : 0.4F;
    }

    public static boolean guiSyncColor() {
        ClickGuiModule m = get();
        return m == null || m.syncColor.get();
    }

    public static Color guiAccentColor() {
        ClickGuiModule m = get();
        return m != null ? m.accentColor.get() : new Color(91, 207, 250);
    }

    public static boolean guiCustomFlow() {
        ClickGuiModule m = get();
        return m != null && m.customFlow.get();
    }

    public static int guiFlowSpeed() {
        ClickGuiModule m = get();
        return m != null ? m.flowSpeed.get().intValue() : 25;
    }

    public static float guiFlowRate() {
        ClickGuiModule m = get();
        return m != null ? m.flowDensity.get().floatValue() / 100.0F : 0.1F;
    }

    public static boolean guiAnimations() {
        ClickGuiModule m = get();
        return m == null || m.animations.get();
    }

    public static float guiAnimSpeed() {
        ClickGuiModule m = get();
        return m != null ? m.animSpeed.get().floatValue() : 1.0F;
    }

    private OpalClickGUI opalGui;

    @Override
    protected void onEnable() {
        if (opalGui == null) {
            opalGui = new OpalClickGUI();
        }
        mc.setScreen(opalGui);
        setEnabled(false);
    }

    @Override
    public String getSuffix() {
        return mode.get();
    }
}
