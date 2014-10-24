/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events;

import com.formationds.commons.events.annotation.EventUtil;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.SystemActivityEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.om.repository.EventRepository;
import com.formationds.om.rest.HttpAuthenticator;
import com.formationds.security.AuthenticationToken;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.lang.reflect.Method;
import java.util.Objects;

/**
 * The event manager is responsible for receiving events and storing them in the event repository.
 *
 */
// TODO: prototype to get us to beta.  not sure if singleton makes sense here in the long run
public enum EventManager {

    /*
     * This initial implementation is extremely simplistic.  It simply takes events and
     */
    INSTANCE;

    private static final Logger LOGGER = LoggerFactory.getLogger(EventManager.class);

    /**
     * Event notification handler interface.  Default implementation in EventManager simply
     * persists the event to the event repository.  More sophisticated implementations may
     * queue events and persist them asynchronously.
     */
    public interface EventNotificationHandler {

        /**
         * Handle the event notification and persist it.
         * <p/>
         * Implementations may throw RuntimeExceptions on error.
         * @param e
         * @return true if successfully handled.  False otherwise.
         */
        public boolean handleEventNotification(Event e);
    }

    /**
     * @param m
     * @param args
     *
     * @return
     */
    // TODO: this is really only valid if using the Event/Audit annotations and a dynamic proxy type approach
    public static boolean eventInterceptor(Method m, Object[] args) {
        Event e = EventUtil.toPersistentEvent(m, args);
        if (e == null)
            return false;

        return INSTANCE.notifyEvent(e);
    }

    /**
     *
     * @param d
     * @param eventArgs
     * @return
     */
    public static boolean notifyEvent(EventDescriptor d, Object... eventArgs) {
        return notifyEvent(d.type(), d.category(), d.severity(), d.key(), eventArgs);
    }

    /**
     *
     * @param t
     * @param c
     * @param s
     * @param key
     * @param eventArgs
     * @return
     */
    public static boolean notifyEvent(EventType t, EventCategory c, EventSeverity s, String key, Object[] eventArgs) {
        Event e = newEvent(t, c, s, key, eventArgs);
        return (e != null ? INSTANCE.notifyEvent(e) : false);
    }

    /**
     *
     * @param t
     * @param c
     * @param s
     * @param key
     * @param eventArgs
     *
     * @return a new event
     */
    public static Event newEvent(EventType t, EventCategory c, EventSeverity s, String key, Object[] eventArgs) {
        switch (t) {
            case SYSTEM_EVENT:
                return new SystemActivityEvent(c, s, key, eventArgs);
            case USER_ACTIVITY:
                AuthenticationToken token = HttpAuthenticator.AUTH_SESSION.get();
                long userId = (token != null ? token.getUserId() : -1);
                return new UserActivityEvent(userId, c, s, key, eventArgs);
            default:
                // TODO: log warning or throw exception on unknown/unsupported type?  Fow now logging.
                // throw new IllegalArgumentException("Unsupported event type (" + t + ")");
                LoggerFactory.getLogger(EventUtil.class).warn("Unsupported event type %s", t);
                return null;
        }
    }

    // TODO: default implementation goes against repository... this probably needs to be a bit smarter.
    private EventNotificationHandler notifier = new EventNotificationHandler() {
        private final EventRepository events = new EventRepository();
        @Override
        public boolean handleEventNotification(Event e) {
            Event persisted = events.save(e);
            return true;
        }
    };

    // key used to guard against someone other than the owner changing the notifier.
    private Object key = null;

    /**
     *
     * @param key
     * @param n
     */
    synchronized public void initEventNotifier(Object key, EventNotificationHandler n) {
        if (this.key != null && !Objects.equals(this.key, key))
            throw new IllegalStateException("Key must match the existing key to change the notifier implementation.");

        notifier = n;
        this.key = key;
    }

    /**
     *
     * @param e
     * @return true if the event is successfully handled.
     */
    public boolean notifyEvent(Event e) {
        try { return notifier.handleEventNotification(e); }
        catch (RuntimeException re) {
            LOGGER.error(String.format("Failed to persist event key={}", key), re);
            return false;
        }
    }

//    public static final long DEFAULT_TIMEOUT = 1;
//    public static final TimeUnit DEFAULT_UNIT = TimeUnit.SECONDS;

    //public boolean notifyEvent(Event e, long timeout, TimeUnit unit) throws InterruptedException;

//    // TODO create adjustable semaphore for adjustable bounded queues (allowing runtime re-configuration as needed).
//    private BlockingQueue<Event> incomingEventQueue = new LinkedBlockingQueue<>(1000);
//
//    // TODO: cache events by timestamp.
//    private ConcurrentSkipListSet<Event> eventCache = new ConcurrentSkipListSet<Event>(new Comparator<Event>() {
//        public int compare(Event e1, Event e2) {
//            return e1.getModifiedTimestamp().compareTo(e2.getModifiedTimestamp());
//        }
//    });
//
//    /**
//     * Notify the event manager of a new event.
//     * <p/>
//     * This is a potentially <em>BLOCKING</em> call if the internal incoming event queue fills faster than
//     * it can be processed.
//     *
//     * @param e
//     * @return true if the event is successfully queued
//     * @throws InterruptedException if event queue operation is interrupted by another thread.
//     */
//    public boolean notify(Event e) throws InterruptedException {
//        incomingEventQueue.put(e);
//        return true;
//    }
//
//    /**
//     * Notify the event manager of a new event.
//     * <p/>
//     * This is a potentially <em>BLOCKING</em> call if the internal incoming event queue fills faster than
//     * it can be processed.
//     *
//     * @param e
//     * @param timeout
//     * @param unit
//     *
//     * @return true if the event is successfully queued.  False if the queue attempt times out before it
//     * can be queued.
//     *
//     * @throws InterruptedException if the event queue operation is interrupted by another thread.
//     */
//    // TODO: Is there a more effective Java8 lambda/streams mechanism to make this async
//    public boolean notify(Event e, long timeout, TimeUnit unit) throws InterruptedException {
//        return incomingEventQueue.offer(e, timeout, unit);
//    }
//
//    // TODO: NOT IMPLEMENTED
//    public List<Event> getEvents() { return null; }

}
