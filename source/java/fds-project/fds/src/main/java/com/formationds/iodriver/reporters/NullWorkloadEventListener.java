package com.formationds.iodriver.reporters;

import java.io.IOException;
import java.util.Collections;
import java.util.Set;

import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.model.VolumeQosSettings;
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
    public void reportOperationExecution(Operation operation)
    {
        // No-op.
    }
}
