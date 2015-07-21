package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;
import java.net.URI;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.AbstractHttpEndpoint;
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
    EndpointT extends AbstractHttpEndpoint<EndpointT, ThisT>>
extends Operation<ThisT, EndpointT>
// @eclipseFormat:on
{
    @Override
    public void accept(EndpointT endpoint,
                       AbstractWorkflowEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        endpoint.visit(getThis(), listener);
    }

    /**
     * Perform the actual operation.
     * 
     * @param endpoint The endpoint to run on.
     * @param connection The connection provided by the endpoint.
     * @param reporter The listener for events.
     * 
     * @throws ExecutionException when an error occurs.
     */
    public abstract void exec(EndpointT endpoint,
                              HttpURLConnection connection,
                              AbstractWorkflowEventListener reporter) throws ExecutionException;

    /**
     * Get the relative URI (to the endpoint) that will be requested.
     * 
     * @return A URI, or {@code null} if the endpoint's base URI is correct.
     */
    public URI getRelativeUri()
    {
        return null;
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
