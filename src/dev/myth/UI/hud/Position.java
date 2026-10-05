package dev.myth.UI.hud;

import dev.myth.UI.setting.Setting;
import java.util.concurrent.ThreadLocalRandom;
import net.minecraft.client.Minecraft;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.Level;
import net.minecraft.world.phys.Vec3;

public final class Position extends TextHudElement {
    public final Setting<Float> safeRange = addSetting(Setting.decimal("safeRange", "SafeRange", 0, 0, 100).unit("k"));
    public final Setting<Boolean> nether = addSetting(Setting.bool("nether", "Nether", true));
    public final Setting<Boolean> fake = addSetting(Setting.bool("fake", "Fake", false).onChange(ignored -> randomize()));
    private int offsetX, offsetZ;
    public Position() { super("position", "Position"); }
    @Override public String getDescription() { return "Shows XYZ and the corresponding Overworld/Nether coordinates."; }
    private void randomize() {
        var player = Minecraft.getInstance().player;
        if (player == null) { offsetX = 13; offsetZ = -37; return; }
        var random = ThreadLocalRandom.current();
        offsetX = (int) (-2 * player.getX() + random.nextDouble() * player.getX() * 4) + 13;
        offsetZ = (int) (-2 * player.getZ() + random.nextDouble() * player.getZ() * 4) - 37;
        if (random.nextBoolean()) offsetX = -offsetX;
        if (random.nextBoolean()) offsetZ = -offsetZ;
    }
    @Override protected String getText() {
        var client = Minecraft.getInstance();
        if (client.player == null || client.level == null) return "XYZ: \u00a7fREDACTED";
        Vec3 position = client.player.position();
        if (safeRange.get() > 0 && position.length() <= safeRange.get() * 1000) return "XYZ: \u00a7fREDACTED";
        if (fake.get()) {
            if (offsetX == 0 && offsetZ == 0) randomize();
            position = position.add(offsetX, 0, offsetZ);
        }
        BlockPos current = BlockPos.containing(position);
        boolean inNether = client.level.dimension().equals(Level.NETHER);
        double multiplier = inNether ? 8 : .125;
        BlockPos other = BlockPos.containing(position.multiply(multiplier, 1, multiplier));
        String text = "XYZ: \u00a7f" + coordinates(current);
        if (nether.get() && !client.level.dimension().equals(Level.END)) text += " (" + coordinates(other) + ")";
        return text;
    }
    private static String coordinates(BlockPos pos) { return pos.getX() + ", " + pos.getY() + ", " + pos.getZ() + "."; }
}
