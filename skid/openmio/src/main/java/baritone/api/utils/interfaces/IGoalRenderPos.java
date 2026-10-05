package baritone.api.utils.interfaces;
import baritone.api.pathing.goals.Goal;
import net.minecraft.util.math.BlockPos;
public interface IGoalRenderPos extends Goal {
    BlockPos getGoalPos();
}
