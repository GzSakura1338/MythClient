package dev.myth.utils;

import java.time.Duration;
import java.util.Objects;

/** Uses a monotonic clock so system time changes cannot affect intervals. */
public final class TimerUtils {
    private long startedAt = System.nanoTime();

    public void reset() {
        startedAt = System.nanoTime();
    }

    public long elapsedMillis() {
        return (System.nanoTime() - startedAt) / 1_000_000L;
    }

    public boolean hasElapsed(Duration duration) {
        Objects.requireNonNull(duration, "duration");
        if (duration.isNegative()) {
            throw new IllegalArgumentException("Duration must not be negative");
        }
        return System.nanoTime() - startedAt >= duration.toNanos();
    }
}
