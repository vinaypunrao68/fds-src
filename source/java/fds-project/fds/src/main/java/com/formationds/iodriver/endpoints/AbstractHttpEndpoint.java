package com.formationds.iodriver.endpoints;

import java.net.HttpURLConnection;
import java.net.URL;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.operations.BaseHttpOperation;
import com.formationds.iodriver.operations.HttpOperation;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public abstract class AbstractHttpEndpoint<ThisT extends AbstractHttpEndpoint<ThisT>>
        extends AbstractBaseHttpEndpoint<ThisT, HttpURLConnection>
        implements HttpEndpoint
{
    public AbstractHttpEndpoint(URL url, Logger logger)
    {
        super(url, logger, HttpURLConnection.class);
    }

    @Override
    public void visit(BaseHttpOperation<HttpURLConnection> operation,
                      AbstractWorkloadEventListener listener) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");
        
        if (operation instanceof HttpOperation)
        {
            visit((HttpOperation)operation, listener);
        }
        else
        {
            operation.accept(getThis(), listener);
        }
    }
    
    protected class CopyHelper
            extends AbstractBaseHttpEndpoint<ThisT, HttpURLConnection>.CopyHelper
    { }

    protected AbstractHttpEndpoint(CopyHelper helper)
    {
        super(helper);
    }
}
