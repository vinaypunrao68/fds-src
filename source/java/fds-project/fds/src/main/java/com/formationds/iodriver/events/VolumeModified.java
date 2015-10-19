package com.formationds.iodriver.events;

import java.time.Instant;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.reporters.BeforeAfter;

public class VolumeModified extends Event<BeforeAfter<Volume>>
{
    public VolumeModified(Instant timestamp, BeforeAfter<Volume> volume)
    {
        super(timestamp);
        
        if (volume == null) throw new NullArgumentException("volume");
        
        _volume = volume;
    }
    
    @Override
    public BeforeAfter<Volume> getData()
    {
        return _volume;
    }
    
    private final BeforeAfter<Volume> _volume;
}
