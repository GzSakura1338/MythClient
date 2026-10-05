package baritone.api.process;
import baritone.api.pathing.goals.Goal;
import net.minecraft.util.math.BlockPos;
public interface IElytraProcess {
    void pathTo(Goal goal);
    void pathTo(BlockPos pos);
}
