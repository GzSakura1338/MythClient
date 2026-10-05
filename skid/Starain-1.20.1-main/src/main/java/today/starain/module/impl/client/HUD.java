package today.starain.module.impl.client;

import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ColorValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;

import java.awt.*;

public class HUD extends ClientModule {

    private final BooleanValue blur = new BooleanValue("Blur", false, this);
    private final BooleanValue limitBlurFps = new BooleanValue("Limit Blur FPS", false, this, blur::get);
    private final NumberValue blurFps = new NumberValue("Blur FPS", 25F, 165F, 60F, 1F, this, () -> blur.get() && limitBlurFps.get());
    private final NumberValue blurRadius = new NumberValue("Blur Radius", 1F, 10F, 3F, 0.01F, this, blur::get);
    private final NumberValue blurIterations = new NumberValue("Blur Iterations", 1F, 5F, 1F, 1F, this, blur::get);
    private final BooleanValue shadow = new BooleanValue("Shadow", false, this);
    private final ColorValue shadowColor = new ColorValue("Shadow Color", new Color(0, 0, 0, 120), this, shadow::get);
    private final BooleanValue chatBackground = new BooleanValue("Chat Background", true, this);
    private final BooleanValue smoothChat = new BooleanValue("Smooth Chat", false, this);
    private final BooleanValue chatHud = new BooleanValue("Chat HUD", false, this);
    private final NumberValue chatWidth = new NumberValue("Chat Width", 80F, 400F, 320F, 1F, this, chatHud::get);
    private final NumberValue chatOpenHeight = new NumberValue("Chat Open Height", 20F, 250F, 130F, 1F, this, chatHud::get);
    private final NumberValue chatClosedHeight = new NumberValue("Chat Closed Height", 20F, 250F, 130F, 1F, this, chatHud::get);
    private final NumberValue chatDuration = new NumberValue("Chat Duration", 1000F, 15000F, 5000F, 500F, this, chatHud::get);

    private final ModeValue themeMode = new ModeValue("Theme Mode", "Gradient", new String[]{"Solid", "Gradient", "Rainbow"}, this);
    private final ColorValue themePrimary = new ColorValue("Theme Primary", new Color(91, 207, 250), this);
    private final ColorValue themeSecondary = new ColorValue("Theme Secondary", new Color(245, 171, 185), this, () -> themeMode.isMode("Gradient"));
    private final BooleanValue trueColor = new BooleanValue("True Color", true, this, () -> themeMode.isMode("Gradient"));
    private final NumberValue themeSpeed = new NumberValue("Theme Speed", 1F, 50F, 25F, 1F, this, () -> !themeMode.isMode("Solid"));
    private final NumberValue themeSpread = new NumberValue("Theme Spread", 0F, 50F, 5F, 1F, this, () -> !themeMode.isMode("Solid"));

    public HUD() {
        setName("HUD");
        setCategory(Category.CLIENT);
    }

    public static boolean isBlurEnabled() {
        return Starain.getInstance().getModuleManager().getModule(HUD.class).blur.get();
    }

    public static boolean isLimitBlurFpsEnabled() {
        return Starain.getInstance().getModuleManager().getModule(HUD.class).limitBlurFps.get();
    }

    public static int getBlurFps() {
        return Starain.getInstance().getModuleManager().getModule(HUD.class).blurFps.get().intValue();
    }

    public static float getBlurRadius() {
        return Starain.getInstance().getModuleManager().getModule(HUD.class).blurRadius.get().floatValue();
    }

    public static int getBlurIterations() {
        return Starain.getInstance().getModuleManager().getModule(HUD.class).blurIterations.get().intValue();
    }

    public static boolean isShadowEnabled() {
        return Starain.getInstance().getModuleManager().getModule(HUD.class).shadow.get();
    }

    public static Color getShadowColor() {
        return Starain.getInstance().getModuleManager().getModule(HUD.class).shadowColor.get();
    }

    public static boolean isChatBackgroundEnabled() {
        Starain starain = Starain.getInstance();
        if (starain == null || starain.getModuleManager() == null) {
            return false;
        }
        HUD hud = starain.getModuleManager().getModule(HUD.class);
        return hud != null && hud.chatBackground.get();
    }

    public static boolean isChatHudEnabled() {
        HUD hud = instance();
        return hud != null && hud.chatHud.get();
    }

    public static float getChatHudWidth() {
        HUD hud = instance();
        return hud == null ? 320F : hud.chatWidth.get().floatValue();
    }

    public static float getChatHudOpenHeight() {
        HUD hud = instance();
        return hud == null ? 130F : hud.chatOpenHeight.get().floatValue();
    }

    public static float getChatHudClosedHeight() {
        HUD hud = instance();
        return hud == null ? 130F : hud.chatClosedHeight.get().floatValue();
    }

    public static float getChatHudDuration() {
        HUD hud = instance();
        return hud == null ? 5000F : hud.chatDuration.get().floatValue();
    }

    private static HUD instance() {
        Starain starain = Starain.getInstance();
        if (starain == null || starain.getModuleManager() == null) {
            return null;
        }
        return starain.getModuleManager().getModule(HUD.class);
    }

    public static boolean isSmoothChatEnabled() {
        Starain starain = Starain.getInstance();
        if (starain == null || starain.getModuleManager() == null) {
            return false;
        }
        HUD hud = starain.getModuleManager().getModule(HUD.class);
        return hud != null && hud.smoothChat.get();
    }

    @Override
    protected void onEnable() {
        setEnabled(false);
    }

    public String getThemeMode() {
        return themeMode.get();
    }

    public Color getThemePrimary() {
        return themePrimary.get();
    }

    public boolean isThemeTrueColor() {
        return trueColor.get();
    }

    public Color getThemeSecondary() {
        return themeSecondary.get();
    }

    public int getThemeSpeed() {
        return themeSpeed.get().intValue();
    }

    public int getThemeSpread() {
        return themeSpread.get().intValue();
    }
}
