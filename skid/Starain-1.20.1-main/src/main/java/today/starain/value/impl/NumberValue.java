package today.starain.value.impl;

import lombok.Getter;
import lombok.Setter;
import today.starain.module.ClientModule;
import today.starain.utils.math.MathUtils;
import today.starain.value.Value;

import java.util.function.Supplier;

@Getter
@Setter
public final class NumberValue extends Value {
    private final double min;
    private final double max;
    private final boolean range;
    private final double step;
    private Double value;
    private Double minValue;
    private Double maxValue;

    public NumberValue(String name, Number min, Number max, Number minValue, Number maxValue, Number step, ClientModule parent, Supplier<Boolean> visible) {
        super(name, parent, visible);
        this.min = min.doubleValue();
        this.max = max.doubleValue();
        this.minValue = clamp(minValue.doubleValue());
        this.maxValue = clamp(maxValue.doubleValue());
        this.range = true;
        this.step = step.doubleValue();
        normalizeRange();
    }

    public NumberValue(String name, Number min, Number max, Number minValue, Number maxValue, Number step, ClientModule parent) {
        this(name, min, max, minValue, maxValue, step, parent, () -> true);
    }

    public NumberValue(String name, Number min, Number max, Number value, Number step, ClientModule parent, Supplier<Boolean> visible) {
        super(name, parent, visible);
        this.min = min.doubleValue();
        this.max = max.doubleValue();
        this.value = clamp(value.doubleValue());
        this.range = false;
        this.step = step.doubleValue();
        this.value = snap(this.value);
    }

    public NumberValue(String name, Number min, Number max, Number value, Number step, ClientModule parent) {
        this(name, min, max, value, step, parent, () -> true);
    }

    public Number get() {
        return range ? MathUtils.getRandomInRange(minValue, maxValue) : value;
    }

    public void setValue(Number value) {
        this.value = snap(clamp(value.doubleValue()));
    }

    public void setMinValue(Number minValue) {
        this.minValue = snap(clamp(minValue.doubleValue()));
        normalizeRange();
    }

    public void setMaxValue(Number maxValue) {
        this.maxValue = snap(clamp(maxValue.doubleValue()));
        normalizeRange();
    }

    private double clamp(double candidate) {
        return MathUtils.clamp(candidate, min, max);
    }

    private double snap(double candidate) {
        return MathUtils.snap(candidate, step);
    }

    private void normalizeRange() {
        if (minValue == null) {
            minValue = min;
        }
        if (maxValue == null) {
            maxValue = max;
        }
        if (minValue > maxValue) {
            double swap = minValue;
            minValue = maxValue;
            maxValue = swap;
        }
    }
}
