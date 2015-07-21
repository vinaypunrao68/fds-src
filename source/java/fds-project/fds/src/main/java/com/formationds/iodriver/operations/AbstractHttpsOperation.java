package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.AbstractHttpsEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * Much the same as {@link AbstractHttpOperation}, but only HTTPS connections are allowed.
 * 
 * @param <ThisT> The implementing class.
 * @param <EndpointT> The type of endpoint this operation can run on.
 */
// @eclipseFormat:off
public abstract class AbstractHttpsOperation<
    ThisT extends AbstractHttpsOperation<ThisT, EndpointT>,
    EndpointT extends AbstractHttpsEndpoint<EndpointT, ThisT>>
extends AbstractHttpOperation<ThisT, EndpointT>
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
                              HttpsURLConnection connection,
                              AbstractWorkflowEventListener reporter) throws ExecutionException;

    @Override
    public final void exec(EndpointT endpoint,
                           HttpURLConnection connection,
                           AbstractWorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (!(connection instanceof HttpsURLConnection))
        {
            throw new IllegalArgumentException("connection must be of type "
                                               + HttpsURLConnection.class.getName());
        }
        if (reporter == null) throw new NullArgumentException("reporter");

        exec(endpoint, (HttpsURLConnection)connection, reporter);
    }
}
