package com.formationds.xdi;

import java.util.List;

import com.formationds.commons.NullArgumentException;
import com.formationds.protocol.BlobDescriptor;

public final class VolumeContents
{
    public VolumeContents(List<BlobDescriptor> blobs, List<String> skippedPrefixes)
    {
        if (blobs == null) throw new NullArgumentException("blobs");
        if (skippedPrefixes == null) throw new NullArgumentException("skippedPrefixes");
        
        _blobs = blobs;
        _skippedPrefixes = skippedPrefixes;
    }
    
    public List<BlobDescriptor> getBlobs()
    {
        return _blobs;
    }
    
    public List<String> getSkippedPrefixes()
    {
        return _skippedPrefixes;
    }
    
    List<BlobDescriptor> _blobs;
    
    List<String> _skippedPrefixes;
}
