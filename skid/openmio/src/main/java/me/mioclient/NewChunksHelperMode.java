package me.mioclient;

import java.awt.Color;
import me.mioclient.module.exploit.NewChunks;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NewChunksHelperMode.class */
public enum NewChunksHelperMode implements EnumSettingHelper {
    NEW("New"),
    OLD("Old") { // from class: me.mioclient.NewChunksHelperMode.Inner_4
        @Override // me.mioclient.NewChunksHelperMode
        public Color[] getColorArray671(NewChunks newChunks) {
            return new Color[]{newChunks.fill.getValue(), newChunks.line3.getValue()};
        }
    },
    BLOCKS("Blocks") { // from class: me.mioclient.NewChunksHelperMode.Inner
        @Override // me.mioclient.NewChunksHelperMode
        public Color[] getColorArray671(NewChunks newChunks) {
            return new Color[]{newChunks.fill3.getValue(), newChunks.line2.getValue()};
        }
    },
    OVERFLOW("Overflow") { // from class: me.mioclient.NewChunksHelperMode.Inner_2
        @Override // me.mioclient.NewChunksHelperMode
        public Color[] getColorArray671(NewChunks newChunks) {
            return new Color[]{newChunks.fill4.getValue(), newChunks.line4.getValue()};
        }
    },
    PLACEHOLDER("Placeholder") { // from class: me.mioclient.NewChunksHelperMode.Inner_3
        @Override // me.mioclient.NewChunksHelperMode
        public Color[] getColorArray671(NewChunks newChunks) {
            return null;
        }
    };

    public final String name;

    NewChunksHelperMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public Color[] getColorArray671(NewChunks newChunks) {
        return new Color[]{newChunks.fill2.getValue(), newChunks.outline.getValue()};
    }
}
