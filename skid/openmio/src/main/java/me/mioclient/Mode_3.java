package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_3.class */
public enum Mode_3 {
    UP(0, -1),
    RIGHT(1, 0),
    DOWN(0, 1),
    LEFT(-1, 0);

    public final PositionData positionData;

    Mode_3(int i, int i2) {
        this.positionData = new PositionData(i, i2);
    }

    public PositionData getPositionData244() {
        return this.positionData;
    }

    public Mode_3 getMode_3245() {
        switch (this) {
            case UP:
                return DOWN;
            case RIGHT:
                return LEFT;
            case DOWN:
                return UP;
            case LEFT:
                return RIGHT;
            default:
                throw new MatchException((String) null, (Throwable) null);
        }
    }
}
