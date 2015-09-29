package com.formationds.iodriver.operations;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;

public interface FdsOperation extends Operation
{
    void accept(FdsEndpoint endpoint,
                WorkloadEventListener listener) throws ExecutionException;
}
