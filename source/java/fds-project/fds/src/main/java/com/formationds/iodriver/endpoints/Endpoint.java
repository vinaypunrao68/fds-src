package com.formationds.iodriver.endpoints;

import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface Endpoint
{
    void visit(Operation operation,
               AbstractWorkflowEventListener listener) throws ExecutionException;
}
