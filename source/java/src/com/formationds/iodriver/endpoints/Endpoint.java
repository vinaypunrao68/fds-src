package com.formationds.iodriver.endpoints;

import com.formationds.commons.patterns.Visitor;
import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;

public abstract class Endpoint<ThisT extends Endpoint<ThisT, OperationT>,
                               OperationT extends Operation<OperationT, ThisT>>
implements Visitor<OperationT, ThisT, ExecutionException>
{
    public abstract ThisT copy();

    public abstract void doVisit(OperationT operation) throws ExecutionException;

    protected abstract class CopyHelper { }

    protected Endpoint() { }

    protected Endpoint(CopyHelper helper)
    {
        if (helper == null) throw new NullArgumentException("helper");
    }
}
