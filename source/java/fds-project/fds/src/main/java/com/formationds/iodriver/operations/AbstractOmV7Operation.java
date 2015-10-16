package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.endpoints.OmV7Endpoint;

/**
 * An operation that runs on the OM.
 */
public abstract class AbstractOmV7Operation extends AbstractHttpsOperation
                                            implements OmV7Operation
{
    @Override
    public void accept(BaseHttpEndpoint<HttpsURLConnection> endpoint,
                       HttpsURLConnection connection,
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (context == null) throw new NullArgumentException("context");

        throw new UnsupportedOperationException(OmV7Endpoint.class.getName() + " required.");
    }
}
