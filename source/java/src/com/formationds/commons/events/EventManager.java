/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events;

import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.SystemActivityEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.security.AuthenticationToken;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Objects;

/**
 * The event manager is responsible for receiving events and storing them in the event repository.
 */
// TODO: prototype to get us to beta.  not sure if singleton makes sense here in the long run
public enum EventManager {

    /*
     * This initial implementation is extremely simplistic.  It simply takes events and
     */
    INSTANCE;

    static final Logger LOG = LoggerFactory.getLogger(EventManager.class);

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
     * Notify the EventManager of a new event with the specified descriptor and event arguments.
     *
     * @param d
     * @param eventArgs
     * @return true if successfully notifying of the event.
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
     * Create a new event of the specified type.
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
                AuthenticationToken token = AuthenticatedRequestContext.getToken();
                long userId = (token != null ? token.getUserId() : -1);
                return new UserActivityEvent(userId, c, s, key, eventArgs);
            default:
                // TODO: log warning or throw exception on unknown/unsupported type?  Fow now logging.
                // throw new IllegalArgumentException("Unsupported event type (" + t + ")");
                LOG.warn("Unsupported event type %s", t);
                return null;
        }
    }

// This needs to be managed outside of commons.events package to prevent
// circular dependency on om.repository.EventRepository
//    public static enum EventNotifier implements EventNotificationHandler {
//        REPOSITORY {
//            private final EventRepository events = new EventRepository();
//            @Override
//            public boolean handleEventNotification(Event e) {
//                Event persisted = events.save(e);
//                return true;
//            }
//        },
//        LOGGER {
//            @Override
//            public boolean handleEventNotification(Event e) {
//                EventManager.LOGGER.info(e.getMessageKey(), e.getMessageArgs());
//                return true;
//            }
//        }
//    }

    // default logger notifier
    private EventNotificationHandler notifier = new EventNotificationHandler() {
        @Override
        public boolean handleEventNotification(Event e) {
            EventManager.LOG.info(e.getMessageKey(), e.getMessageArgs());
            return true;
        }
    };

    // key used to guard against someone other than the owner changing the notifier.
    private Object key = null;

    /**
     * Initialize the event manager with the specified notification handler implementation.  The caller must
     * have a valid key in order to change the notification handler.  Assuming the key is private to the
     * object that owns it, this ensures that the notification handler configuration is managed from a
     * single location and prevents unintended modifications.
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
            LOG.error(String.format("Failed to persist event key={}", key), re);
            return false;
        }
    }

//
//    // TODO: NOT IMPLEMENTED
//    public List<Event> getEvents() { return null; }

}
