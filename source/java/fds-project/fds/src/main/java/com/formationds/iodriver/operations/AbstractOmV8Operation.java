package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.endpoints.OmV8Endpoint;

public abstract class AbstractOmV8Operation extends AbstractHttpsOperation
                                            implements OmV8Operation
{
    @Override
    public void accept(BaseHttpEndpoint<HttpsURLConnection> endpoint,
                       HttpsURLConnection connection,
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (context == null) throw new NullArgumentException("context");

        throw new UnsupportedOperationException(OmV8Endpoint.class.getName() + " required.");
    }
}
