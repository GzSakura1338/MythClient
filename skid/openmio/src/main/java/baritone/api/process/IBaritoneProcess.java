package baritone.api.process;
public interface IBaritoneProcess {
    boolean isActive();
    PathingCommand onTick(boolean calcFailed, boolean isSafeToCancel);
    boolean isTemporary();
    void onLostControl();
    double priority();
}
