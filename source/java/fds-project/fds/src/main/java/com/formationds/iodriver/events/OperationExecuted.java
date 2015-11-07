package com.formationds.iodriver.events;

import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.operations.Operation;

public class OperationExecuted extends Event<Operation>
{
    public OperationExecuted(Instant timestamp, Operation operation)
    {
        super(timestamp);
        
        if (operation == null) throw new NullArgumentException("operation");
        
        _operation = operation;
    }
    
    @Override
    public Operation getData()
    {
        return _operation;
    }
    
    private final Operation _operation;
}
