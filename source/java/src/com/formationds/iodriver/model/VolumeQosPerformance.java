package com.formationds.iodriver.model;

import java.time.Instant;

public final class VolumeQosPerformance
{
    public VolumeQosPerformance()
    {
        this(0, null, null);
    }

    public VolumeQosPerformance(int ops, Instant start, Instant stop)
    {
        if (ops < 0)
        {
            throw new IllegalArgumentException("ops of " + ops + " is not >= 0.");
        }
        if (start == null && stop != null)
        {
            throw new IllegalArgumentException("Cannot have a stop time without a start.");
        }
        else if (start != null)
        {
            if (stop != null && start.isAfter(stop))
            {
                throw new IllegalArgumentException("Cannot stop before starting: " + start + " > " + stop);
            }
        }
        
        _ops = ops;
        _start = start;
        _stop = stop;
    }

    public int addOps(int ops)
    {
        if (ops < 0)
        {
            throw new IllegalArgumentException(ops + " ops is less than minimum of 0.");
        }
        verifyNotStopped();

        return _ops += ops;
    }

    public int getOps()
    {
        verifyStopped();
        
        return _ops;
    }

    public Instant getStart()
    {
        verifyStarted();
        
        return _start;
    }

    public Instant getStop()
    {
        verifyStopped();
        
        return _stop;
    }
    
    public boolean isStarted()
    {
        return _start != null;
    }
    
    public boolean isStopped()
    {
        return _stop != null;
    }
    
    public Instant startNow()
    {
        verifyNotStarted();
        
        return _start = Instant.now();
    }
    
    public Instant stopNow()
    {
        verifyStarted();
        verifyNotStopped();
        
        return _stop = Instant.now();
    }

    public void verifyNotStarted()
    {
        if (isStarted())
        {
            throw new IllegalStateException("Already started.");
        }
    }
    
    public void verifyNotStopped()
    {
        if (isStopped())
        {
            throw new IllegalStateException("Already stopped.");
        }
    }
    
    public void verifyStarted()
    {
        if (!isStarted())
        {
            throw new IllegalStateException("Not started yet.");
        }
    }
    
    public void verifyStopped()
    {
        if (!isStopped())
        {
            throw new IllegalStateException("Not stopped yet.");
        }
    }
    
    private int _ops;

    private Instant _start;

    private Instant _stop;
}
