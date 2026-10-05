package baritone.api.process;
import baritone.api.pathing.goals.Goal;
public interface ICustomGoalProcess {
    void setGoal(Goal goal);
    void setGoalAndPath(Goal goal);
}
