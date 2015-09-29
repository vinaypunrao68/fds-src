package com.formationds.iodriver.operations;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;

public interface Operation
{
    void accept(Endpoint endpoint,
                WorkloadEventListener listener) throws ExecutionException;
}
