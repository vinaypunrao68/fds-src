package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.endpoints.OmEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * An operation that runs on the OM.
 */
public abstract class AbstractOmOperation extends AbstractHttpsOperation
                                          implements OmOperation
{
    @Override
    public void accept(BaseHttpEndpoint<HttpsURLConnection> endpoint,
                       HttpsURLConnection connection,
                       AbstractWorkflowEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (connection == null) throw new NullArgumentException("connection");
        if (listener == null) throw new NullArgumentException("listener");

        throw new UnsupportedOperationException(OmEndpoint.class.getName() + " required.");
    }
}
