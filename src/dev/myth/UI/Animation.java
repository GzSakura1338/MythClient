package dev.myth.UI;

/** Mio HUDHelper_2/Progress interpolation: 1 - (1 - 2 * dt * speed)^4. */
final class Animation {
    private float value;
    Animation(float value) { this.value = value; }
    float value() { return value; }
    void snap(float value) { this.value = value; }
    float update(float target, float speed, float delta, boolean eased) {
        float factor = Math.clamp(2 * delta * speed, 0, 1);
        if (eased) factor = 1 - (float) Math.pow(1 - factor, 4);
        value += (target - value) * factor;
        if (Math.abs(target - value) < .01f || delta >= 1f / 15f) value = target;
        return value;
    }
}
