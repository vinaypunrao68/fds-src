package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.OmV7Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public interface OmV7Operation extends HttpsOperation
{
    void accept(OmV7Endpoint endpoint,
                HttpsURLConnection connection,
                AbstractWorkloadEventListener listener) throws ExecutionException;
}
