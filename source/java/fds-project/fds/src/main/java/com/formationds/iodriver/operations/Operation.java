package com.formationds.iodriver.operations;

import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface Operation
{
    void accept(Endpoint endpoint,
                AbstractWorkflowEventListener listener) throws ExecutionException;
}
