package com.formationds.fdsdiff.operations;

import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.operations.AbstractFdsOperation;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.FdsOperation;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public final class ListVolumes extends AbstractFdsOperation implements FdsOperation
{
    @Override
    public void accept(FdsEndpoint endpoint,
                       AbstractWorkloadEventListener listener) throws ExecutionException
    {
    }
}
