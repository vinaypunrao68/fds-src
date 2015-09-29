package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;

public abstract class AbstractOmV8Operation extends AbstractHttpsOperation
                                            implements OmV8Operation
{
    @Override
    public void accept(BaseHttpEndpoint<HttpsURLConnection> endpoint,
                       HttpsURLConnection connection,
                       WorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (listener == null) throw new NullArgumentException("listener");

        throw new UnsupportedOperationException(OmV8Endpoint.class.getName() + " required.");
    }
}
