package com.formationds.iodriver.endpoints;

import java.net.HttpURLConnection;
import java.net.URL;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.operations.BaseHttpOperation;
import com.formationds.iodriver.operations.HttpOperation;

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
                      WorkloadContext context) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (context == null) throw new NullArgumentException("context");
        
        if (operation instanceof HttpOperation)
        {
            visit((HttpOperation)operation, context);
        }
        else
        {
            operation.accept(getThis(), context);
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
