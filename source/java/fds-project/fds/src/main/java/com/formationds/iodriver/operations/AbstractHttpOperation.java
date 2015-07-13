package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * Basic operation that runs on an HTTP endpoint.
 * 
 * @param <ThisT> The implementing class.
 * @param <EndpointT> The type of endpoint this operation can run on.
 */
// @eclipseFormat:off
public abstract class AbstractHttpOperation<
    ThisT extends AbstractHttpOperation<ThisT, EndpointT>,
    EndpointT extends BaseHttpEndpoint<EndpointT, ? super ThisT, ? extends HttpURLConnection>>
extends AbstractBaseHttpOperation<ThisT, EndpointT, HttpURLConnection>
implements HttpOperation<ThisT, EndpointT>
// @eclipseFormat:on
{
    /**
     * Perform the actual operation without opening a connection first.
     *
     * @param endpoint The endpoint to run on.
     * @param reporter The listener for events.
     *
     * @throws ExecutionException when an error occurs.
     */
    public void exec(EndpointT endpoint,
                     AbstractWorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (reporter == null) throw new NullArgumentException("reporter");
        
        throw new UnsupportedOperationException("Connectionless exec not implemented.");
    }
    
    /**
     * Get whether this operation needs a connection.
     *
     * @return Whether a connection should be provided when executing this operation.
     */
    public boolean getNeedsConnection()
    {
        return true;
    }

    /**
     * Get a typed reference to {@code this} in parent classes.
     * 
     * @return {@code this}.
     */
    @SuppressWarnings("unchecked")
    protected final ThisT getThis()
    {
        return (ThisT)this;
    }
}
