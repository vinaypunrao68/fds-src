package com.formationds.iodriver.reporters;

import java.io.IOException;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.operations.Operation;

public class BaseWorkloadEventListener extends AbstractWorkloadEventListener
{
    public BaseWorkloadEventListener(Logger logger)
    {
        super(logger);
    }
    
    @Override
    public void close() throws IOException
    {
        // No-op.
    }

    @Override
    public BaseWorkloadEventListener copy()
    {
        return new BaseWorkloadEventListener(new CopyHelper());
    }

    @Override
    public void reportOperationExecution(Operation operation)
    {
        if (operation == null) throw new NullArgumentException("operation");
        
        operationExecuted.send(operation);
    }
    
    protected BaseWorkloadEventListener(CopyHelper copyHelper)
    {
        super(copyHelper);
    }
}
