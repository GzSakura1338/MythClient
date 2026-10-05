package today.starain.event.api;

import today.starain.event.api.annotations.EventPriority;
import today.starain.event.api.annotations.EventTarget;

import java.lang.reflect.Method;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Event bus.
 *
 * <p>Performance notes: {@link #call(Event)} runs on the hot path (every client tick and every
 * rendered frame). Handlers are compiled once at {@link #register} time into an immutable
 * {@link Listener}{@code []} snapshot per event type, so dispatch is a single map lookup plus a
 * flat array walk — no per-dispatch {@code setAccessible} call and no per-dispatch map lookups for
 * the target instance, which the previous reflective implementation paid on every single event.
 */
public class EventManager {

    private static final Comparator<Listener> PRIORITY_COMPARATOR =
            Comparator.comparingInt(listener -> listener.priority);
    private static final Listener[] EMPTY = new Listener[0];
    // Per concrete event type -> priority-sorted snapshot iterated lock-free on the hot path.
    private final Map<Class<?>, Listener[]> listeners = new ConcurrentHashMap<>();

    private EventManager() {

    }

    // 如果直接去new一个对象这个zkm的indy没法混 直接是明文调用 所以要搞一个newInstance方法
    public static EventManager netInstance() {
        return new EventManager();
    }

    /**
     * Registers one or more objects, associating their {@link EventTarget}-annotated methods with
     * the event bus.
     *
     * @param objects One or more objects to register.
     */
    public void register(Object... objects) {
        for (Object object : objects) {
            register(object);
        }
    }

    /**
     * Registers an object, associating its {@link EventTarget}-annotated methods with the event bus.
     *
     * @param obj The object to register.
     */
    public void register(Object obj) {
        for (Method method : obj.getClass().getDeclaredMethods()) {
            if (method.getAnnotation(EventTarget.class) == null || method.getParameterCount() != 1) {
                continue;
            }

            Class<?> eventClass = method.getParameterTypes()[0];
            EventPriority priorityAnnotation = method.getAnnotation(EventPriority.class);
            int priority = (priorityAnnotation != null) ? priorityAnnotation.value() : 10;

            method.setAccessible(true); // Done once here, not on every dispatch.
            Listener listener = new Listener(obj, method, priority);

            listeners.compute(eventClass, (key, current) -> {
                List<Listener> merged = new ArrayList<>();
                if (current != null) {
                    Collections.addAll(merged, current);
                }
                merged.add(listener);
                merged.sort(PRIORITY_COMPARATOR);
                return merged.toArray(new Listener[0]);
            });
        }
    }

    /**
     * Unregisters an object, removing all of its handlers from the event bus.
     *
     * @param obj The object to unregister.
     */
    public void unregister(Object obj) {
        for (Map.Entry<Class<?>, Listener[]> entry : listeners.entrySet()) {
            Listener[] current = entry.getValue();
            List<Listener> retained = new ArrayList<>(current.length);
            for (Listener listener : current) {
                if (listener.target != obj) {
                    retained.add(listener);
                }
            }
            if (retained.size() != current.length) {
                entry.setValue(retained.toArray(new Listener[0]));
            }
        }
    }

    /**
     * Dispatches an event to its registered handlers, respecting their priorities.
     *
     * @param event The event to dispatch.
     * @return The same event instance after all handlers have run.
     */
    public Event call(Event event) {
        Listener[] handlers = listeners.getOrDefault(event.getClass(), EMPTY);
        for (Listener listener : handlers) {
            try {
                listener.method.invoke(listener.target, event);
            } catch (Exception e) {
                throw new RuntimeException("Failed to dispatch event "
                        + event.getClass().getName() + " to " + listener.method, e);
            }
        }
        return event;
    }

    public void unregisterAll() {
        listeners.clear();
    }

    /**
     * A pre-resolved handler: an already-accessible method bound to its target instance.
     */
    private record Listener(Object target, Method method, int priority) {
    }
}
