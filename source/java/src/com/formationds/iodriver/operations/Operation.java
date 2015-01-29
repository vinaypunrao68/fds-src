package com.formationds.iodriver.operations;

import com.formationds.commons.patterns.Visitable;
import com.formationds.iodriver.endpoints.Endpoint;

public abstract class Operation<ThisT extends Operation<ThisT, EndpointT>,
                                EndpointT extends Endpoint<EndpointT, ThisT>>
implements Visitable<EndpointT, ThisT, ExecutionException>
{
    public int getCost()
    {
        return 1;
    }
    
    public abstract void accept(EndpointT endpoint) throws ExecutionException;
}
