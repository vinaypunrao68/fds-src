package com.formationds.iodriver.endpoints;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.operations.Operation;

/**
 * A generic endpoint.
 * 
 * @param <ThisT> The implementing class.
 * @param <OperationT> The type of operation that may be visited.
 */
// @eclipseFormat:off
public abstract class AbstractEndpoint<ThisT extends AbstractEndpoint<ThisT, OperationT>,
                                       OperationT extends Operation<OperationT, ? super ThisT>>
implements Endpoint<ThisT, OperationT>
// @eclipseFormat:on
{
    /**
     * Perform a deep copy.
     * 
     * @return A duplicate of this object.
     */
    public abstract ThisT copy();

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
        public final Class<OperationT> baseOperationClass =
                AbstractEndpoint.this._baseOperationClass;
    }

    /**
     * Constructor.
     */
    protected AbstractEndpoint(Class<OperationT> baseOperationClass)
    {
        if (baseOperationClass == null) throw new NullArgumentException("baseOperationClass");

        _baseOperationClass = baseOperationClass;
    }

    /**
     * Copy constructor.
     * 
     * @param helper Object holding copied values to assign to the new object.
     */
    protected AbstractEndpoint(CopyHelper helper)
    {
        if (helper == null) throw new NullArgumentException("helper");

        _baseOperationClass = helper.baseOperationClass;
    }

    private final Class<OperationT> _baseOperationClass;
}
