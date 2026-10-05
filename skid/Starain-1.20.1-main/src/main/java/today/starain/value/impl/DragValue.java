package today.starain.value.impl;

import lombok.Getter;
import lombok.Setter;
import today.starain.module.ClientModule;
import today.starain.utils.animation.SmoothAnimationTimer;
import today.starain.value.Value;

import java.util.function.Supplier;

@Getter
public final class DragValue extends Value {

    private final float defaultX;
    private final float defaultY;
    private final SmoothAnimationTimer renderXAnimation;
    private final SmoothAnimationTimer renderYAnimation;
    @Setter
    private float width;
    @Setter
    private float height;
    @Setter
    private float contentOffsetX;
    @Setter
    private float contentOffsetY;

    public DragValue(String name, ClientModule parent, float defaultX, float defaultY, Supplier<Boolean> visibility) {
        super(name, parent, visibility);
        this.defaultX = defaultX;
        this.defaultY = defaultY;
        this.renderXAnimation = new SmoothAnimationTimer(defaultX, defaultX, 0.1F);
        this.renderYAnimation = new SmoothAnimationTimer(defaultY, defaultY, 0.1F);
    }

    public DragValue(String name, ClientModule parent, float defaultX, float defaultY) {
        this(name, parent, defaultX, defaultY, () -> true);
    }

    public float resolveWidth(float fallback) {
        return width <= 0.0F ? fallback : width;
    }

    public float resolveHeight(float fallback) {
        return height <= 0.0F ? fallback : height;
    }

    public void setTargetX(float x) {
        renderXAnimation.target = x;
    }

    public void setTargetY(float y) {
        renderYAnimation.target = y;
    }

    public void updateRenderPosition() {
        renderXAnimation.update(true);
        renderYAnimation.update(true);
    }

    public float getX() {
        return renderXAnimation.value;
    }

    public void setX(float x) {
        renderXAnimation.target = x;
        renderXAnimation.value = x;
    }

    public float getY() {
        return renderYAnimation.value;
    }

    public void setY(float y) {
        renderYAnimation.target = y;
        renderYAnimation.value = y;
    }

    public void resetToDefault() {
        setX(defaultX);
        setY(defaultY);
    }
}
