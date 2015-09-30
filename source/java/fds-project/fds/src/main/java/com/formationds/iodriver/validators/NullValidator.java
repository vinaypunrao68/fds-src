package com.formationds.iodriver.validators;

import java.io.Closeable;
import java.io.IOException;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.reporters.WorkloadEventListener;

/**
 * Doesn't actually validate anything.
 */
public final class NullValidator implements Validator
{
    @Override
    public boolean isValid(Closeable context, WorkloadEventListener listener)
    {
        return true;
    }

    @Override
    public Closeable newContext(WorkloadEventListener listener)
    {
        if (listener == null) throw new NullArgumentException("listener");
        
        return new NullCloseable();
    }
    
    private static final class NullCloseable implements Closeable
    {
        @Override
        public void close() throws IOException
        {
            // No-op.
        }
    }
}
