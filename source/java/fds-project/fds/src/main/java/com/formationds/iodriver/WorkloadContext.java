package com.formationds.iodriver;

import static com.formationds.commons.util.ExceptionHelper.tunnelNow;
import static com.formationds.commons.util.ExceptionHelper.untunnel;

import java.io.Closeable;
import java.io.IOException;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Observer;
import com.formationds.commons.patterns.Subject;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.events.Event;

public class WorkloadContext implements Closeable
{
    public WorkloadContext(Logger logger)
    {
        if (logger == null) throw new NullArgumentException("logger");
        
        _events = new ConcurrentHashMap<>();
        _logger = logger;
        _lifecycle = Lifecycle.CONSTRUCTED;
        _lifecycleLock = new Object();
    }
    
    public final void close() throws IOException
    {
        untunnel(this::ensureLifecycle, Lifecycle.CLOSED, IOException.class);
    }
    
    public final Logger getLogger()
    {
        return _logger;
    }
    
    public <EventT extends Event<T>, T> Closeable subscribe(Class<EventT> eventType,
                                                            Observer<EventT> observer)
    {
        if (eventType == null) throw new NullArgumentException("eventType");
        if (observer == null) throw new NullArgumentException("observer");
        
        ensureLifecycle(Lifecycle.READY);
        
        Optional<Closeable> registrationToken = subscribeIfProvided(eventType, observer);
        if (registrationToken.isPresent())
        {
            return registrationToken.get();
        }
        else
        {
            throw new IllegalArgumentException(eventType.getName() + " is not provided.");
        }
    }
    
    public <EventT extends Event<T>, T> void registerEvent(Class<EventT> eventType,
                                                           Subject<EventT> subject)
    {
        if (eventType == null) throw new NullArgumentException("eventType");
        if (subject == null) throw new NullArgumentException("subject");
        
        if (_events.putIfAbsent(eventType, subject) != null)
        {
            throw new IllegalArgumentException("event " + eventType.getName() + " already exists.");
        }
    }
    
    public <EventT extends Event<T>, T> Optional<Closeable> subscribeIfProvided(
            Class<EventT> eventType,
            Observer<EventT> observer)
    {
        if (eventType == null) throw new NullArgumentException("eventType");
        if (observer == null) throw new NullArgumentException("observer");
        
        ensureLifecycle(Lifecycle.READY);
        
        Subject<? extends Event<?>> subject = _events.get(eventType);
        if (subject == null)
        {
            return Optional.empty();
        }
        else
        {
            @SuppressWarnings("unchecked")
            Subject<EventT> typedSubject = (Subject<EventT>)subject;
            return Optional.of(typedSubject.register(observer));
        }
    }
    
    public <EventT extends Event<? super T>, T> boolean sendIfRegistered(Class<EventT> eventType,
                                                                         EventT event)
    {
        if (eventType == null) throw new NullArgumentException("eventType");
        if (event == null) throw new NullArgumentException("event");
        
        ensureLifecycle(Lifecycle.READY);
        
        Subject<? extends Event<?>> subject = _events.get(eventType);
        if (subject == null)
        {
            return false;
        }
        else
        {
            @SuppressWarnings("unchecked")
            Subject<EventT> typedSubject = (Subject<EventT>)subject;
            typedSubject.send(event);
            
            return true;
        }
    }
    
    public <EventT extends Event<? super T>, T> boolean sendIfRegistered(EventT event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        @SuppressWarnings("unchecked")
        Class<EventT> eventClass = (Class<EventT>)event.getClass();
        return sendIfRegistered(eventClass, event);
    }

    private static enum Lifecycle
    {
        // CONSTRUCTING omitted because it cannot be accurate.
        CONSTRUCTED,
        SETTING_UP,
        READY,
        CLOSED
    }
    
    protected void closeInternal() throws IOException
    {
        // No-op.
    }
    
    protected final void ensureLifecycle(Lifecycle state)
    {
        if (state == null) throw new NullArgumentException("state");
        
        if (_lifecycle.compareTo(state) < 0)
        {
            synchronized (_lifecycleLock)
            {
                while (_lifecycle.compareTo(state) < 0)
                {
                    switch (_lifecycle)
                    {
                    case CONSTRUCTED:
                        _lifecycle = Lifecycle.SETTING_UP;
                        break;
                    case SETTING_UP:
                        setUp();
                        _lifecycle = Lifecycle.READY;
                        break;
                    case READY:
                        tunnelNow(this::closeInternal, IOException.class);
                        _lifecycle = Lifecycle.CLOSED;
                        break;
                    default:
                        throw new IllegalStateException(
                                "Cannot progress beyond lifecycle " + _lifecycle + ".");
                    }
                }
            }
        }
    }
    
    protected void setUp()
    {
        // No-op.
    }

    private final ConcurrentMap<Class<? extends Event<?>>, Subject<? extends Event<?>>> _events;
    
    private final Logger _logger;
    
    // volatile needed for double-checked locking.
    private volatile Lifecycle _lifecycle;
    
    private final Object _lifecycleLock;
}
