package com.formationds.commons.util;

import java.io.IOException;
import java.io.InputStream;
import java.util.Iterator;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;

public final class StreamInputStream extends InputStream
{
    public StreamInputStream(Stream<Byte> source)
    {
        if (source == null) throw new NullArgumentException("source");
        
        _source = source.iterator();
    }
    
    @Override
    public int read() throws IOException
    {
        if (_source.hasNext())
        {
            return _source.next();
        }
        else
        {
            return -1;
        }
    }
    
    private final Iterator<Byte> _source;
}
