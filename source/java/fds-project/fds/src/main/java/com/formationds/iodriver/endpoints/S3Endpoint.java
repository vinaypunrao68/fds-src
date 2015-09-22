package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.net.MalformedURLException;

import org.apache.log4j.Appender;
import org.apache.log4j.Layout;
import org.apache.log4j.LogManager;
import org.apache.log4j.spi.ErrorHandler;
import org.apache.log4j.spi.Filter;
import org.apache.log4j.spi.LoggingEvent;

import com.amazonaws.SDKGlobalConfiguration;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.OmBaseEndpoint.AuthToken;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.S3Operation;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

/**
 * An S3 service endpoint.
 */
public final class S3Endpoint implements Endpoint
{
    /**
     * Constructor.
     * 
     * @param s3url Base URL for future requests.
     * @param omEndpoint An Orchestration Manager endpoint used to authenticate.
     * @param logger Log messages here.
     * 
     * @throws MalformedURLException when {@code s3url} is not a valid absolute URL.
     */
    public S3Endpoint(String s3url,
                      OmBaseEndpoint<?> omEndpoint,
                      Logger logger) throws MalformedURLException
    {
        if (s3url == null) throw new NullArgumentException("s3url");
        if (omEndpoint == null) throw new NullArgumentException("omEndpoint");
        if (logger == null) throw new NullArgumentException("logger");

        _omEndpoint = omEndpoint;
        _s3url = s3url;
        _logger = logger;
    }

    public S3Endpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        return new S3Endpoint(copyHelper);
    }

    /**
     * Perform an operation on this endpoint.
     * 
     * @param operation The operation to perform.
     * @param listener Report operations here.
     * 
     * @throws ExecutionException when an error occurs executing {@code operation}.
     */
    // @eclipseFormatter:off
    public void visit(S3Operation operation,
                      AbstractWorkloadEventListener listener) throws ExecutionException
    // @eclipseFormatter:on
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");

        AmazonS3Client client;
        try
        {
            client = getClient();
        }
        catch (IOException e)
        {
            throw new ExecutionException("Error getting S3 client.", e);
        }
        
        operation.accept(this, client, listener);
    }
    
    @Override
    public void visit(Operation operation,
                      AbstractWorkloadEventListener listener) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");
        
        if (operation instanceof S3Operation)
        {
            visit((S3Operation)operation, listener);
        }
        else
        {
            operation.accept(this, listener);
        }
    }

    /**
     * Extend this class to allow deep copies even when the private memebers of superclasses aren't
     * available.
     */
    protected class CopyHelper
    {
        public final Logger logger = _logger;
        public final OmBaseEndpoint<?> omEndpoint = _omEndpoint.copy();
        public final String s3url = _s3url;
    }

    /**
     * Copy constructor.
     * 
     * @param helper An object holding the copied values to assign to the new endpoint.
     */
    protected S3Endpoint(CopyHelper helper)
    {
        _logger = helper.logger;
        _omEndpoint = helper.omEndpoint;
        _s3url = helper.s3url;
    }

    /**
     * Get the S3 client.
     * 
     * @return The S3 client.
     * 
     * @throws IOException when an error occurs creating the client.
     */
    protected final AmazonS3Client getClient() throws IOException
    {
        if (_client == null)
        {
            AuthToken authToken = _omEndpoint.getAuthToken();
            
            // TODO: Logging should be configurable, but for now, we need this to be quiet.
            LogManager.getRootLogger().removeAllAppenders();
            LogManager.getRootLogger().addAppender(new Appender()
            {
                @Override public void addFilter(Filter newFilter) { }
                @Override public void clearFilters() { }
                @Override public void close() { }
                @Override public void doAppend(LoggingEvent event) { }
                @Override public void setErrorHandler(ErrorHandler errorHandler) { }
                @Override public void setLayout(Layout layout) { }
                @Override public void setName(String name) { }

                @Override
                public ErrorHandler getErrorHandler()
                {
                    return null;
                }

                @Override
                public Filter getFilter()
                {
                    return null;
                }

                @Override
                public Layout getLayout()
                {
                    return null;
                }

                @Override
                public String getName()
                {
                    return "FdsNullAppender";
                }

                @Override
                public boolean requiresLayout()
                {
                    return false;
                }
            });
            
//            @SuppressWarnings("unchecked")
//            List<org.apache.log4j.Logger> loggers =
//                    Collections.<org.apache.log4j.Logger>list(LogManager.getCurrentLoggers());
//            for (org.apache.log4j.Logger logger : loggers)
//            {
//                if (logger.getName().startsWith("com.amazon"))
//                {
//                    logger.setLevel(Level.OFF);
//                }
//            }
            
//            org.apache.log4j.Logger logger =
//                    LogManager.getLogger(com.amazonaws.internal.config.InternalConfig.class);
//            logger.setLevel(Level.OFF);
            
            _client = new AmazonS3Client(new BasicAWSCredentials(_omEndpoint.getUsername(),
                                                                 authToken.toString()));
            _client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
            _client.setEndpoint(_s3url);
        }
        return _client;
    }

    /**
     * The S3 client used for operations. {@code null} if not authenticated yet.
     */
    private AmazonS3Client _client;

    /**
     * Log here.
     */
    private final Logger _logger;

    /**
     * OM endpoint used to authenticate.
     */
    private final OmBaseEndpoint<?> _omEndpoint;

    /**
     * The base URL for requests.
     */
    private final String _s3url;

    /**
     * Static constructor.
     */
    static
    {
        System.setProperty(SDKGlobalConfiguration.DISABLE_CERT_CHECKING_SYSTEM_PROPERTY, "true");
    }
}
