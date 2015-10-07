package com.formationds.iodriver.operations;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.OmV8Endpoint;

public interface OmV8Operation extends HttpsOperation
{
    void accept(OmV8Endpoint endpoint,
                HttpsURLConnection connection,
                WorkloadContext context) throws ExecutionException;
}
