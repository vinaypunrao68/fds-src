package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.AbstractHttpsEndpoint;
import com.formationds.iodriver.reporters.VerificationReporter;

// @eclipseFormat:off
public abstract class AbstractHttpsOperation<
    ThisT extends AbstractHttpsOperation<ThisT, EndpointT>,
    EndpointT extends AbstractHttpsEndpoint<EndpointT, ThisT>>
extends AbstractHttpOperation<ThisT, EndpointT>
// @eclipseFormat:on
{
    @Override
    public void accept(EndpointT endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        endpoint.visit(getThis());
    }

    public abstract void exec(EndpointT endpoint,
                              HttpsURLConnection connection,
                              VerificationReporter reporter) throws ExecutionException;

    @Override
    public final void exec(EndpointT endpoint,
                           HttpURLConnection connection,
                           VerificationReporter reporter) throws ExecutionException
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
