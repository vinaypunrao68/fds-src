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
        
        _source = source;
    }
    
    @Override
    public int read() throws IOException
    {
        Iterator<Byte> iterator = _getIterator();
        if (iterator.hasNext())
        {
            return iterator.next();
        }
        else
        {
            return -1;
        }
    }
    
    private Iterator<Byte> _iterator;
    
    private final Stream<Byte> _source;
    
    private Iterator<Byte> _getIterator()
    {
        if (_iterator == null)
        {
            _iterator = _source.iterator();
        }
        return _iterator;
    }
}
