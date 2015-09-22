package com.formationds.iodriver.operations;

import java.net.URI;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public interface BaseHttpOperation<ConnectionT> extends Operation
{
    /**
     * Perform the actual operation.
     * 
     * @param endpoint The endpoint to run on.
     * @param connection A connection according to {@link #getRelativeUri()} and
     *                   {@link #getRequestMethod()}.
     * @param reporter The listener for events.
     * 
     * @throws ExecutionException when an error occurs.
     */
    void accept(BaseHttpEndpoint<ConnectionT> endpoint,
                ConnectionT connection,
                AbstractWorkloadEventListener reporter) throws ExecutionException;

    /**
     * Get the type of connection this operation requires.
     *
     * @return The current property value.
     */
    Class<ConnectionT> getConnectionType();
    
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
