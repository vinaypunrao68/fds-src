package com.formationds.iodriver.endpoints;

import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * A target that operations can be executed on.
 */
public interface Endpoint
{
    /**
     * Execute an operation.
     *
     * @param operation The operation to execute.
     * @param listener Report progress here.
     *
     * @throws ExecutionException when an error occurs.
     */
    void visit(Operation operation,
               AbstractWorkflowEventListener listener) throws ExecutionException;
}
