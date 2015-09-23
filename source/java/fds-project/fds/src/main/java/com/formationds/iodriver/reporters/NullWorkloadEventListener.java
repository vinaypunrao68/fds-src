package com.formationds.iodriver.reporters;

import java.io.IOException;

import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.operations.Operation;

public class NullWorkloadEventListener extends AbstractWorkloadEventListener
{
    public NullWorkloadEventListener(Logger logger)
    {
        super(logger);
    }

    @Override
    public void close() throws IOException
    {
        // No-op.
    }
    
    @Override
    public NullWorkloadEventListener copy()
    {
        return new NullWorkloadEventListener(new CopyHelper());
    }
    
    @Override
    public void reportOperationExecution(Operation operation)
    {
        // No-op.
    }
    
    protected NullWorkloadEventListener(CopyHelper copyHelper)
    {
        super(copyHelper);
    }
}
