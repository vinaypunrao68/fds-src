package com.formationds.iodriver.endpoints;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.ProtocolException;
import java.net.URI;
import java.net.URL;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.HttpOperation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public abstract class AbstractHttpEndpoint extends AbstractBaseHttpEndpoint<HttpURLConnection>
                                           implements HttpEndpoint
{
    public AbstractHttpEndpoint(URL url, Logger logger)
    {
        super(url, logger);
    }
    
    /**
     * Visit an operation.
     * 
     * @param operation The operation to visit.
     * @param listener The listener to report progress to.
     * 
     * @throws ExecutionException when an error occurs.
     */
    // @eclipseFormat:off
    public void visit(HttpOperation operation,
                      AbstractWorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");

        HttpURLConnection connection;
        try
        {
            URI relativeUri = operation.getRelativeUri();
            if (relativeUri == null)
            {
                connection = openConnection();
            }
            else
            {
                connection = openRelativeConnection(relativeUri);
            }
        }
        catch (IOException e)
        {
            throw new ExecutionException(e);
        }

        String requestMethod = operation.getRequestMethod();
        try
        {
            connection.setRequestMethod(requestMethod);
        }
        catch (ProtocolException e)
        {
            throw new ExecutionException(
                    "Error setting request method to " + javaString(requestMethod) + ".", e);
        }

        try
        {
            operation.accept(getThis(), connection, listener);
        }
        finally
        {
            connection.disconnect();
        }
    }

    protected class CopyHelper extends AbstractBaseHttpEndpoint<HttpURLConnection>.CopyHelper
    { }

    protected AbstractHttpEndpoint(CopyHelper helper)
    {
        super(helper);
    }
    
    protected abstract AbstractHttpEndpoint getThis();
}
