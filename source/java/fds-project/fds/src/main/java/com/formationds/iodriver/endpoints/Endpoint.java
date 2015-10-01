package com.formationds.iodriver.endpoints;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.WorkloadEventListener;

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
               WorkloadEventListener listener) throws ExecutionException;
}
