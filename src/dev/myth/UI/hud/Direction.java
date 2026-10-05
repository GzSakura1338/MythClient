package dev.myth.UI.hud;

import net.minecraft.client.Minecraft;
import net.minecraft.util.Mth;

public final class Direction extends TextHudElement {
    private static final String[] NAMES = {"South", "SouthWest", "West", "NorthWest", "North", "NorthEast", "East", "SouthEast"};
    private static final String[] AXES = {"+Z", "-X +Z", "-X", "-X -Z", "-Z", "+X -Z", "+X", "+X +Z"};
    public Direction() { super("direction", "Direction"); }
    @Override public String getDescription() { return "Shows the camera's compass direction and yaw."; }
    @Override protected String getText() {
        float yaw = Minecraft.getInstance().gameRenderer.mainCamera().yRot();
        int index = Math.floorMod(Mth.floor(yaw * 8 / 360 + .5), 8);
        return NAMES[index] + "\u00a7f (" + (int) Mth.wrapDegrees(yaw) + ", " + AXES[index] + ")";
    }
}
