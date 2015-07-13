package com.formationds.iodriver.operations;

import java.net.URI;

import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface BaseHttpOperation<ConnectionT> extends Operation
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
    void accept(BaseHttpEndpoint<ConnectionT> endpoint,
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
