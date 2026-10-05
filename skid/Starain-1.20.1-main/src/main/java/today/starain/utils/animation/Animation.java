package today.starain.utils.animation;

import lombok.Getter;
import lombok.Setter;

public class Animation {

    private final Easing easing;
    @Setter
    @Getter
    private long duration;
    @Getter
    @Setter
    private long millis;
    private long startTime;

    @Getter
    private float startValue;
    private float destinationValue;
    @Setter
    @Getter
    private float value;
    @Setter
    @Getter
    private boolean finished;

    public Animation(Easing easing, long duration) {
        this.easing = easing;
        this.startTime = System.currentTimeMillis();
        this.duration = duration;
    }

    public void run(float destinationValue) {
        this.millis = System.currentTimeMillis();
        if (this.destinationValue != destinationValue) {
            this.destinationValue = destinationValue;
            this.reset();
        } else {
            this.finished = this.millis - this.duration > this.startTime || this.value == destinationValue;
            if (this.finished) {
                this.value = destinationValue;
                return;
            }
        }

        float result = this.easing.getFunction().apply(this.getProgress());
        if (this.duration == 0L) {
            this.value = destinationValue;
        } else if (this.value > destinationValue) {
            this.value = this.startValue - (this.startValue - destinationValue) * result;
        } else {
            this.value = this.startValue + (destinationValue - this.startValue) * result;
        }

        if (Float.isNaN(value) || !Float.isFinite(value)) {
            this.value = destinationValue;
        }
    }

    public float getProgress() {
        return (float) (System.currentTimeMillis() - this.startTime) / (float) this.duration;
    }

    public void reset() {
        this.startTime = System.currentTimeMillis();
        this.startValue = value;
        this.finished = false;
    }

    public void setStartValue(float startValue) {
        this.startValue = startValue;
        this.value = startValue;
    }

}
