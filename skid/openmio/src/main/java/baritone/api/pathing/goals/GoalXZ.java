package baritone.api.pathing.goals;
import net.minecraft.util.math.Vec3d;
public class GoalXZ implements Goal {
    private final int x, z;
    public GoalXZ(int x, int z) { this.x = x; this.z = z; }
    public int getX() { return x; }
    public int getZ() { return z; }
    public static GoalXZ fromDirection(Vec3d origin, double yaw, double distance) {
        return new GoalXZ((int) origin.getX(), (int) origin.getZ());
    }
}
