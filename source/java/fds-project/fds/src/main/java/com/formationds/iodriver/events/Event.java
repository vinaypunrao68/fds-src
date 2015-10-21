package com.formationds.iodriver.events;

import java.time.Instant;

import com.formationds.commons.NullArgumentException;

public abstract class Event<T>
{
    public abstract T getData();
    
    public final Instant getTimestamp()
    {
        return _timestamp;
    }
    
    protected Event(Instant timestamp)
    {
        if (timestamp == null) throw new NullArgumentException("timestamp");
        
        _timestamp = timestamp;
    }
    
    private final Instant _timestamp;
}