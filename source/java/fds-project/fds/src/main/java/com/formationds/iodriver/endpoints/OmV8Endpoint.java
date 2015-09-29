package com.formationds.iodriver.endpoints;

import java.net.MalformedURLException;
import java.net.URI;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.operations.BaseHttpOperation;
import com.formationds.iodriver.operations.OmV8Operation;
import com.formationds.iodriver.reporters.WorkloadEventListener;

public class OmV8Endpoint extends OmBaseEndpoint<OmV8Endpoint>
{
    public OmV8Endpoint(URI uri, String username, String password, Logger logger, boolean trusting)
            throws MalformedURLException
    {
        super(uri, username, password, logger, trusting);
    }
    
    public OmV8Endpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        return new OmV8Endpoint(copyHelper);
    }
    
    protected OmV8Endpoint(CopyHelper copyHelper)
    {
        super(copyHelper);
    }
    
    @Override
    protected final void typedVisit(BaseHttpOperation<HttpsURLConnection> operation,
                                    HttpsURLConnection connection,
                                    WorkloadEventListener listener)
            throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (connection == null) throw new NullArgumentException("connection");
        if (listener == null) throw new NullArgumentException("listener");
        
        if (operation instanceof OmV8Operation)
        {
            ((OmV8Operation)operation).accept(this, connection, listener);
        }
        else
        {
            operation.accept(this, connection, listener);
        }
    }
}
