package today.starain.ui.clickgui.opal.component;

import lombok.Getter;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;

public final class Scroller {

    @Getter
    private final Animation animation = new Animation(Easing.EASE_OUT_EXPO, 250);
    private float value;

    public void onScroll(float maxOffset) {
        this.value = Math.min(0, Math.max(-maxOffset, this.value));
        this.animation.run(this.value);
    }

    public void addScroll(double verticalScroll, float maxOffset) {
        this.value += (float) (verticalScroll * 50);
        this.value = Math.max(-maxOffset, this.value);
        this.value = Math.min(0, this.value);
        this.animation.run(this.value);
    }
}
