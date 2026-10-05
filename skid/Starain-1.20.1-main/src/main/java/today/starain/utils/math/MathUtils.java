package today.starain.utils.math;

import java.math.BigDecimal;
import java.math.RoundingMode;
import java.util.Random;

public class MathUtils {

    public static float getRandomInRange(float min, float max) {
        return min >= max ? min : new Random().nextFloat() * (max - min) + min;
    }

    public static double getRandomInRange(double min, double max) {
        return min >= max ? min : new Random().nextDouble() * (max - min) + min;
    }

    public static int getRandomInRange(int min, int max) {
        return min >= max ? min : new Random().nextInt() * (max - min) + min;
    }

    public static long getRandomInRange(long min, long max) {
        return min >= max ? min : new Random().nextLong() * (max - min) + min;
    }

    public static Double interpolate(double oldValue, double newValue, double interpolationValue) {
        return (oldValue + (newValue - oldValue) * interpolationValue);
    }

    public static float interpolateFloat(float oldValue, float newValue, double interpolationValue) {
        return interpolate(oldValue, newValue, (float) interpolationValue).floatValue();
    }

    public static int interpolateInt(int oldValue, int newValue, double interpolationValue) {
        return interpolate(oldValue, newValue, (float) interpolationValue).intValue();
    }

    public static double clamp(double value, double min, double max) {
        return Math.max(min, Math.min(max, value));
    }

    public static double snap(double value, double step) {
        if (step <= 0.0D) {
            return value;
        }
        return BigDecimal.valueOf(value / step)
                .setScale(0, RoundingMode.HALF_UP)
                .multiply(BigDecimal.valueOf(step))
                .doubleValue();
    }
}
