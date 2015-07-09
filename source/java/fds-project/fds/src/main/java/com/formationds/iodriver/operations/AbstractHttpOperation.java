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
     * Get the relative URI (to the endpoint) that will be requested.
     * 
     * @return A URI, or {@code null} if the endpoint's base URI is correct.
     */
    public URI getRelativeUri()
    {
        return null;
    }

    /**
     * Get the HTTP request method to use.
     *
     * @return The HTTP request method to use. {@code null} may be returned if
     *         {@link #getNeedsConnection()} returns {@code false}.
     */
    public abstract String getRequestMethod();

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
