package dev.myth.event.api;

import dev.myth.event.api.annotations.EventPriority;
import dev.myth.event.api.annotations.EventTarget;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;

/** Exact event type dispatch, in ascending priority order, matching Starain's API. */
public final class EventManager {
    private static final Listener[] EMPTY = new Listener[0];
    private static final Comparator<Listener> ORDER = Comparator.comparingInt(Listener::priority);
    private final Map<Class<?>, Listener[]> listeners = new ConcurrentHashMap<>();

    private EventManager() {
    }

    // Preserve the factory spelling used by Starain.
    public static EventManager netInstance() {
        return new EventManager();
    }

    public void register(Object... objects) {
        Objects.requireNonNull(objects, "objects");
        for (Object object : objects) {
            register(object);
        }
    }

    public synchronized void register(Object object) {
        Objects.requireNonNull(object, "object");
        var additions = new ArrayList<Listener>();
        for (Method method : object.getClass().getDeclaredMethods()) {
            if (!method.isAnnotationPresent(EventTarget.class) || method.getParameterCount() != 1) {
                continue;
            }
            if (!Event.class.isAssignableFrom(method.getParameterTypes()[0])) {
                throw new IllegalArgumentException("Handler parameter must implement Event: " + method);
            }
            method.setAccessible(true);
            EventPriority priority = method.getAnnotation(EventPriority.class);
            additions.add(new Listener(object, method, priority == null ? Priority.MEDIUM : priority.value()));
        }
        for (Listener addition : additions) {
            Class<?> eventType = addition.method().getParameterTypes()[0];
            Listener[] current = listeners.getOrDefault(eventType, EMPTY);
            boolean duplicate = Arrays.stream(current).anyMatch(listener ->
                    listener.target() == object && listener.method().equals(addition.method()));
            if (!duplicate) {
                Listener[] updated = Arrays.copyOf(current, current.length + 1);
                updated[current.length] = addition;
                Arrays.sort(updated, ORDER);
                listeners.put(eventType, updated);
            }
        }
    }

    public synchronized void unregister(Object object) {
        Objects.requireNonNull(object, "object");
        for (var entry : listeners.entrySet()) {
            Listener[] retained = Arrays.stream(entry.getValue())
                    .filter(listener -> listener.target() != object).toArray(Listener[]::new);
            if (retained.length == 0) {
                listeners.remove(entry.getKey());
            } else if (retained.length != entry.getValue().length) {
                listeners.put(entry.getKey(), retained);
            }
        }
    }

    public <T extends Event> T call(T event) {
        Objects.requireNonNull(event, "event");
        for (Listener listener : listeners.getOrDefault(event.getClass(), EMPTY)) {
            try {
                listener.method().invoke(listener.target(), event);
            } catch (InvocationTargetException exception) {
                throw new RuntimeException("Failed to dispatch " + event.getClass().getName()
                        + " to " + listener.method(), exception.getCause());
            } catch (ReflectiveOperationException exception) {
                throw new RuntimeException("Failed to dispatch " + event.getClass().getName()
                        + " to " + listener.method(), exception);
            }
        }
        return event;
    }

    public synchronized void unregisterAll() {
        listeners.clear();
    }

    private record Listener(Object target, Method method, int priority) {
    }
}
