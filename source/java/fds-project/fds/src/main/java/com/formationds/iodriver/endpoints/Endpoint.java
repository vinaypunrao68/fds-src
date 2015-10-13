package com.formationds.iodriver.endpoints;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.operations.Operation;

/**
 * A target that operations can be executed on.
 */
public interface Endpoint
{
    /**
     * Execute an operation.
     *
     * @param operation The operation to execute.
     *
     * @throws ExecutionException when an error occurs.
     */
    void visit(Operation operation,
               WorkloadContext context) throws ExecutionException;
}
