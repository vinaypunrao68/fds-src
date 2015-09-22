package com.formationds.iodriver.operations;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public interface Operation
{
    void accept(Endpoint endpoint,
                AbstractWorkloadEventListener listener) throws ExecutionException;
}
