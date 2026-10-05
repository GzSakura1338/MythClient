package baritone.api;
import java.awt.Color;
import java.util.function.Consumer;
import net.minecraft.text.Text;
/** Stub of baritone.api.Settings holding only the settings this mod touches. */
public class Settings {
    public static class Setting<T> { public T value; }
    public Setting<Boolean> chatControl = new Setting<>();
    public Setting<Boolean> disconnectOnArrival = new Setting<>();
    public Setting<Boolean> freeLook = new Setting<>();
    public Setting<Boolean> blockFreeLook = new Setting<>();
    public Setting<Boolean> elytraFreeLook = new Setting<>();
    public Setting<Boolean> censorCoordinates = new Setting<>();
    public Setting<Boolean> censorRanCommands = new Setting<>();
    public Setting<Boolean> useMessageTag = new Setting<>();
    public Setting<Boolean> antiCheatCompatibility = new Setting<>();
    public Setting<Boolean> assumeExternalAutoTool = new Setting<>();
    public Setting<Boolean> assumeSafeWalk = new Setting<>();
    public Setting<Boolean> assumeStep = new Setting<>();
    public Setting<Boolean> assumeWalkOnWater = new Setting<>();
    public Setting<Color> colorBestPathSoFar = new Setting<>();
    public Setting<Color> colorBlocksToBreak = new Setting<>();
    public Setting<Color> colorBlocksToPlace = new Setting<>();
    public Setting<Color> colorBlocksToWalkInto = new Setting<>();
    public Setting<Color> colorCurrentPath = new Setting<>();
    public Setting<Color> colorGoalBox = new Setting<>();
    public Setting<Color> colorInvertedGoalBox = new Setting<>();
    public Setting<Color> colorMostRecentConsidered = new Setting<>();
    public Setting<Color> colorNextPath = new Setting<>();
    public Setting<Float> goalRenderLineWidthPixels = new Setting<>();
    public Setting<Float> pathRenderLineWidthPixels = new Setting<>();
    public Setting<String> prefix = new Setting<>();
    public Setting<Consumer<Text>> logger = new Setting<>();
    public void initialize() {}
}
