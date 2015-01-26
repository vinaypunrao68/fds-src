package com.formationds.iodriver.operations;

import java.io.IOException;
import java.net.HttpURLConnection;

import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.HttpEndpoint;

public abstract class HttpOperation extends Operation
{
    public abstract void exec(HttpURLConnection connection);
    
    @Override
    public void runOn(Endpoint endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (!(endpoint instanceof HttpEndpoint))
        {
            throw new IllegalArgumentException("endpoint must be an instance of "
                                               + HttpEndpoint.class.getName());
        }

        runOn((HttpEndpoint)endpoint);
    }
    
    public void runOn(HttpEndpoint endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        
        endpoint.exec(this);
    }
}
