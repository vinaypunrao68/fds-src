package com.formationds.iodriver.operations;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public interface FdsOperation extends Operation
{
    void accept(FdsEndpoint endpoint,
                AbstractWorkloadEventListener listener) throws ExecutionException;
}
