package me.mioclient;

import java.awt.Color;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_7.class */
public enum Mode_7 {
    NoShape(new Color(0, 0, 0)),
    ZShape(new Color(204, 102, 102)),
    SShape(new Color(102, 204, 102)),
    LineShape(new Color(102, 102, 204)),
    TShape(new Color(204, 204, 102)),
    SquareShape(new Color(204, 102, 204)),
    LShape(new Color(102, 204, 204)),
    MirroredLShape(new Color(218, 170, 0));

    public final Color color;

    Mode_7(Color color) {
        this.color = color;
    }

    public Color getColor1125() {
        return this.color;
    }
}
