package baritone.api.selection;
import net.minecraft.util.math.BlockPos;
public interface ISelection {
    BlockPos min();
    BlockPos max();
}
