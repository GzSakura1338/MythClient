package baritone.api.process;
import baritone.api.pathing.goals.Goal;
public class PathingCommand {
    public final Goal goal;
    public final PathingCommandType commandType;
    public PathingCommand(Goal goal, PathingCommandType commandType) { this.goal = goal; this.commandType = commandType; }
}
