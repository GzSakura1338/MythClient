package today.starain.utils.client;

import net.minecraft.network.chat.Style;
import net.minecraft.network.chat.TextColor;
import net.minecraft.util.FormattedCharSequence;
import today.starain.Starain;
import today.starain.ui.Theme;

import java.awt.Color;
import java.util.ArrayList;
import java.util.List;

public final class ChatFlow {

    private static final int[] TARGET = Starain.NAME.codePoints().toArray();

    private ChatFlow() {
    }

    public static FormattedCharSequence recolor(FormattedCharSequence text) {
        if (text == null) {
            return FormattedCharSequence.EMPTY;
        }

        List<Integer> cps = new ArrayList<>();
        List<Style> styles = new ArrayList<>();
        text.accept((index, style, codePoint) -> {
            cps.add(codePoint);
            styles.add(style);
            return true;
        });

        int at = indexOf(cps);
        if (at < 0) {
            return text;
        }

        List<FormattedCharSequence> parts = new ArrayList<>(cps.size());
        for (int i = 0; i < cps.size(); i++) {
            Style style = styles.get(i);
            if (i >= at && i < at + TARGET.length) {
                Color c = Theme.accent(-(i - at));
                style = style.withColor(TextColor.fromRgb(c.getRGB() & 0xFFFFFF));
            }
            parts.add(FormattedCharSequence.forward(new String(Character.toChars(cps.get(i))), style));
        }
        return FormattedCharSequence.composite(parts);
    }

    private static int indexOf(List<Integer> cps) {
        if (TARGET.length == 0) {
            return -1;
        }
        outer:
        for (int i = 0; i + TARGET.length <= cps.size(); i++) {
            for (int j = 0; j < TARGET.length; j++) {
                if (cps.get(i + j) != TARGET[j]) {
                    continue outer;
                }
            }
            return i;
        }
        return -1;
    }
}
