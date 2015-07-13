package com.formationds.iodriver.operations;

import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface FdsOperation extends Operation
{
    void accept(FdsEndpoint endpoint,
                AbstractWorkflowEventListener listener) throws ExecutionException;
}
