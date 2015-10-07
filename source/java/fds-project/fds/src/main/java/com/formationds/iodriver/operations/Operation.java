package com.formationds.iodriver.operations;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.Endpoint;

public interface Operation
{
    void accept(Endpoint endpoint,
                WorkloadContext context) throws ExecutionException;
}
