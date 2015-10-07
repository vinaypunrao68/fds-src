package com.formationds.iodriver.events;

import java.time.Instant;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;

public class VolumeAdded extends Event<Volume>
{
    public VolumeAdded(Instant timestamp, Volume volume)
    {
        super(timestamp);
        
        if (volume == null) throw new NullArgumentException("volume");
        
        _volume = volume;
    }
    
    @Override
    public Volume getData()
    {
        return _volume;
    }
    
    private final Volume _volume;
}
