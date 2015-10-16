package com.formationds.iodriver.operations;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.FdsEndpoint;

public interface FdsOperation extends Operation
{
    void accept(FdsEndpoint endpoint,
                WorkloadContext context) throws ExecutionException;
}
