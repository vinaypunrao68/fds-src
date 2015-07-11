package com.formationds.iodriver.operations;

import java.net.URI;

import com.formationds.iodriver.endpoints.AbstractHttpEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface BaseHttpOperation<ThisT extends BaseHttpOperation<ThisT, EndpointT, ConnectionT>,
                                   EndpointT extends AbstractHttpEndpoint<EndpointT, ? super ThisT>,
                                   ConnectionT>
extends Operation<ThisT, EndpointT>
{
    /**
     * Perform the actual operation.
     * 
     * @param endpoint The endpoint to run on.
     * @param connection The connection provided by the endpoint.
     * @param reporter The listener for events.
     * 
     * @throws ExecutionException when an error occurs.
     */
    void exec(EndpointT endpoint,
              ConnectionT connection,
              AbstractWorkflowEventListener reporter) throws ExecutionException;
    
    /**
     * Get the relative URI (to the endpoint) that will be requested.
     * 
     * @return A URI, or {@code null} if the endpoint's base URI is correct.
     */
    URI getRelativeUri();
    
    /**
     * Get the HTTP request method to use.
     *
     * @return The HTTP request method to use. {@code null} may be returned if
     *         {@link #getNeedsConnection()} returns {@code false}.
     */
    String getRequestMethod();
}
