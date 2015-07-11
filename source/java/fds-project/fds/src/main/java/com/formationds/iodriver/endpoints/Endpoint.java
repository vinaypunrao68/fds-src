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
                               OperationT extends Operation<OperationT, ? super ThisT>>
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
     * Get the least specific type this endpoint can service.
     *
     * @return A class.
     */
    public final Class<OperationT> getBaseOperationClass()
    {
        return _baseOperationClass;
    }

    /**
     * Extend this class in order to allow deep copies even when superclass private members aren't
     * available.
     */
    protected abstract class CopyHelper
    {
        public final Class<OperationT> baseOperationClass = Endpoint.this._baseOperationClass;
    }

    /**
     * Constructor.
     */
    protected Endpoint(Class<OperationT> baseOperationClass)
    {
        if (baseOperationClass == null) throw new NullArgumentException("baseOperationClass");

        _baseOperationClass = baseOperationClass;
    }

    /**
     * Copy constructor.
     * 
     * @param helper Object holding copied values to assign to the new object.
     */
    protected Endpoint(CopyHelper helper)
    {
        if (helper == null) throw new NullArgumentException("helper");

        _baseOperationClass = helper.baseOperationClass;
    }

    private final Class<OperationT> _baseOperationClass;
}
