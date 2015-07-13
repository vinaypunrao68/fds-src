package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.iodriver.endpoints.OmEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface OmOperation extends HttpsOperation
{
    void accept(OmEndpoint endpoint,
                HttpsURLConnection connection,
                AbstractWorkflowEventListener listener) throws ExecutionException;
}
