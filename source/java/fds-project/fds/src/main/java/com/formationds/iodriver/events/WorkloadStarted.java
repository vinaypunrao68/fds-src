package com.formationds.iodriver.events;

import java.time.Instant;

public class WorkloadStarted extends Event<Void>
{
    public WorkloadStarted(Instant timestamp)
    {
        super(timestamp);
    }
    
    @Override
    public Void getData()
    {
        return null;
    }
}
