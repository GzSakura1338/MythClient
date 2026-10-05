package dev.myth.event.api;

public interface Cancellable {
    boolean isCancelled();

    void setCancelled(boolean cancelled);
}
