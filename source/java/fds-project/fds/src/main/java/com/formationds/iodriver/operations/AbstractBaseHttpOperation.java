package com.formationds.iodriver.operations;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public abstract class AbstractBaseHttpOperation<
        ThisT extends AbstractBaseHttpOperation<ThisT, EndpointT, ConnectionT>,
        EndpointT extends BaseHttpEndpoint<EndpointT, ? super ThisT, ? extends ConnectionT>,
        ConnectionT>
extends AbstractOperation<ThisT, EndpointT>
implements BaseHttpOperation<ThisT, EndpointT, ConnectionT>
{
    @Override
    public void accept(EndpointT endpoint,
                       AbstractWorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (reporter == null) throw new NullArgumentException("reporter");
        
        endpoint.doVisit(getThis(), reporter);
    }
}
