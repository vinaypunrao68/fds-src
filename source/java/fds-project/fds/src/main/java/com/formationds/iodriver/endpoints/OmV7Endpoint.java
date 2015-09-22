package com.formationds.iodriver.endpoints;

import java.net.MalformedURLException;
import java.net.URI;

import javax.net.ssl.HttpsURLConnection;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.operations.BaseHttpOperation;
import com.formationds.iodriver.operations.OmV7Operation;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

/**
 * An endpoint connecting to an FDS Orchestration Manager.
 */
// @eclipseFormat:off
public class OmV7Endpoint extends OmBaseEndpoint<OmV7Endpoint>
// @eclipseFormat:on
{
    public OmV7Endpoint(URI uri, String username, String password, Logger logger, boolean trusting)
            throws MalformedURLException
    {
        super(uri, username, password, logger, trusting);
    }
    
    public OmV7Endpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        return new OmV7Endpoint(copyHelper);
    }

    protected OmV7Endpoint(CopyHelper copyHelper)
    {
        super(copyHelper);
    }
    
    @Override
    protected final void typedVisit(BaseHttpOperation<HttpsURLConnection> operation,
                                    HttpsURLConnection connection,
                                    AbstractWorkloadEventListener listener)
            throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (connection == null) throw new NullArgumentException("connection");
        if (listener == null) throw new NullArgumentException("listener");
        
        if (operation instanceof OmV7Operation)
        {
            ((OmV7Operation)operation).accept(this, connection, listener);
        }
        else
        {
            operation.accept(this, connection, listener);
        }
    }
}
