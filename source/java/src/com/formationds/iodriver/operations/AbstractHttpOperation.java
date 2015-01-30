package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;
import java.net.URI;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.AbstractHttpEndpoint;
import com.formationds.iodriver.reporters.VerificationReporter;

// @eclipseFormat:off
public abstract class AbstractHttpOperation<
    ThisT extends AbstractHttpOperation<ThisT, EndpointT>,
    EndpointT extends AbstractHttpEndpoint<EndpointT, ThisT>>
extends Operation<ThisT, EndpointT>
// @eclipseFormat:on
{
    @Override
    public void accept(EndpointT endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        endpoint.visit(getThis());
    }

    public abstract void exec(EndpointT endpoint,
                              HttpURLConnection connection,
                              VerificationReporter reporter) throws ExecutionException;

    public URI getRelativeUri()
    {
        return null;
    }

    @SuppressWarnings("unchecked")
    protected final ThisT getThis()
    {
        return (ThisT)this;
    }
}
