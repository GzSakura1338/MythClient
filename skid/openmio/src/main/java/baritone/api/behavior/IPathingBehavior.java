package baritone.api.behavior;
import baritone.api.utils.interfaces.IGoalRenderPos;
public interface IPathingBehavior {
    boolean isPathing();
    IGoalRenderPos getGoal();
    void cancelEverything();
    boolean forceCancel();
}
