package com.formationds.iodriver.endpoints;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.VisitorWithArg;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * A generic endpoint.
 * 
 * @param <ThisT> The implementing class.
 * @param <OperationT> The type of operation that may be visited.
 */
// @eclipseFormat:off
public abstract class Endpoint<ThisT extends Endpoint<ThisT, OperationT>,
                               OperationT extends Operation<OperationT, ThisT>>
implements VisitorWithArg<OperationT, ThisT, AbstractWorkflowEventListener, ExecutionException>
// @eclipseFormat:on
{
    /**
     * Perform a deep copy.
     * 
     * @return A duplicate of this object.
     */
    public abstract ThisT copy();

    @Override
    // @eclipseFormat:off
    public abstract void doVisit(OperationT operation,
                                 AbstractWorkflowEventListener listener) throws ExecutionException;
    // @eclipseFormat:on

    /**
     * Extend this class in order to allow deep copies even when superclass private members aren't
     * available.
     */
    protected abstract class CopyHelper
    {}

    /**
     * Constructor.
     */
    protected Endpoint()
    {}

    /**
     * Copy constructor.
     * 
     * @param helper Object holding copied values to assign to the new object.
     */
    protected Endpoint(CopyHelper helper)
    {
        if (helper == null) throw new NullArgumentException("helper");
    }
}
