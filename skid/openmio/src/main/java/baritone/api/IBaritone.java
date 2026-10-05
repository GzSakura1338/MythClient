package baritone.api;
import baritone.api.process.ICustomGoalProcess;
import baritone.api.process.IElytraProcess;
import baritone.api.behavior.IPathingBehavior;
import baritone.api.pathing.calc.IPathingControlManager;
import baritone.api.selection.ISelectionManager;
import baritone.api.event.listener.IGameEventListener;
public interface IBaritone {
    ICustomGoalProcess getCustomGoalProcess();
    IElytraProcess getElytraProcess();
    IPathingBehavior getPathingBehavior();
    IPathingControlManager getPathingControlManager();
    ISelectionManager getSelectionManager();
    IGameEventListener getGameEventHandler();
}
