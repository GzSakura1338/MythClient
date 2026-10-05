package me.mioclient.feature;

import java.util.function.Supplier;
import me.mioclient.GameMode;
import me.mioclient.GameStopwatchSearchHelper419;
import me.mioclient.GameStopwatchSearchHelper419_2;
import me.mioclient.PresetEnumSettingHelper;
import me.mioclient.PresetHelper_5;
import me.mioclient.PresetSearchHelper419;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Game.class */
public class Game extends PresetEnumSettingHelper {
    public GameMode gameMode;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/feature/Game$Inner_2.class */
    class Inner_2 extends PresetSearchHelper419 {
        public Inner_2(PresetEnumSettingHelper presetEnumSettingHelper, Supplier supplier) {
            super(presetEnumSettingHelper, (Supplier<String>) supplier);
        }

        @Override // me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
        public void do20(double d, double d2, int i) {
            super.do20(d, d2, i);
            if (is92(d, d2) && i == 0) {
                Game.this.do1634(GameMode.values()[(Game.this.gameMode.ordinal() + 1) % GameMode.values().length]);
            }
        }
    }

    public Game() {
        super("Game");
        this.gameMode = GameMode.TETRIS;
        register(new GameStopwatchSearchHelper419_2(this, 0));
        register(new Inner_2(this, () -> {
            return this.gameMode.getName();
        }));
    }

    public void do1634(GameMode gameMode) {
        PresetHelper_5 gameStopwatchSearchHelper419;
        this.gameMode = gameMode;
        switch (gameMode) {
            case TETRIS:
                gameStopwatchSearchHelper419 = new GameStopwatchSearchHelper419_2(this, 0);
                break;
            case SNAKE:
                gameStopwatchSearchHelper419 = new GameStopwatchSearchHelper419(this, 0);
                break;
            default:
                throw new MatchException((String) null, (Throwable) null);
        }
        PresetHelper_5 presetHelper_5 = gameStopwatchSearchHelper419;
        synchronized (this.registry) {
            this.registry.set(0, presetHelper_5);
        }
    }

    @Override // me.mioclient.PresetEnumSettingHelper
    public int get1635() {
        return 110;
    }
}
