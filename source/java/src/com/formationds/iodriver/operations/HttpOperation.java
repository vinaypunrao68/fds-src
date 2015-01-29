package com.formationds.iodriver.operations;

import java.net.HttpURLConnection;

import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.endpoints.HttpEndpoint;

public abstract class HttpOperation extends Operation<HttpOperation, HttpEndpoint>
{
    public abstract void exec(HttpURLConnection connection);
    
    @Override
    public void accept(HttpEndpoint endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        endpoint.visit(this);
    }
}
