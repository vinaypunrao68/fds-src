package com.formationds.iodriver.endpoints;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.VisitorWithArg;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.WorkflowEventListener;

// @eclipseFormat:off
public abstract class Endpoint<ThisT extends Endpoint<ThisT, OperationT>,
                               OperationT extends Operation<OperationT, ThisT>>
implements VisitorWithArg<OperationT, ThisT, WorkflowEventListener, ExecutionException>
// @eclipseFormat:on
{
    public abstract ThisT copy();

    // @eclipseFormat:off
    public abstract void doVisit(OperationT operation,
                                 WorkflowEventListener listener) throws ExecutionException;
    // @eclipseFormat:on

    protected abstract class CopyHelper
    {}

    protected Endpoint()
    {}

    protected Endpoint(CopyHelper helper)
    {
        if (helper == null) throw new NullArgumentException("helper");
    }
}
