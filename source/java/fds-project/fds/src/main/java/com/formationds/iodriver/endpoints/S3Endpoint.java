package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.net.MalformedURLException;

import com.amazonaws.SDKGlobalConfiguration;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint.AuthToken;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.S3Operation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * An S3 service endpoint.
 */
public final class S3Endpoint extends Endpoint<S3Endpoint, S3Operation>
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
                      OrchestrationManagerEndpoint omEndpoint,
                      Logger logger) throws MalformedURLException
    {
        if (s3url == null) throw new NullArgumentException("s3url");
        if (omEndpoint == null) throw new NullArgumentException("omEndpoint");
        if (logger == null) throw new NullArgumentException("logger");

        _omEndpoint = omEndpoint;
        _s3url = s3url;
        _logger = logger;
    }

    @Override
    public S3Endpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        return new S3Endpoint(copyHelper);
    }

    @Override
    // @eclipseFormat:off
    public void doVisit(S3Operation operation,
                        AbstractWorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");

        visit(operation, listener);
    }

    /**
     * Get the Orchestration Manager endpoint used to authenticate.
     * 
     * @return An OM endpoint.
     */
    public OrchestrationManagerEndpoint getOmEndpoint()
    {
        return _omEndpoint;
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
                      AbstractWorkflowEventListener listener) throws ExecutionException
    // @eclipseFormatter:on
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");

        try
        {
            operation.exec(this, getClient(), listener);
        }
        catch (IOException e)
        {
            throw new ExecutionException(e);
        }
    }

    /**
     * Extend this class to allow deep copies even when the private memebers of superclasses aren't
     * available.
     */
    protected class CopyHelper extends Endpoint<S3Endpoint, S3Operation>.CopyHelper
    {
        public final Logger logger = _logger;
        public final OrchestrationManagerEndpoint omEndpoint = _omEndpoint.copy();
        public final String s3url = _s3url;
    }

    /**
     * Copy constructor.
     * 
     * @param helper An object holding the copied values to assign to the new endpoint.
     */
    protected S3Endpoint(CopyHelper helper)
    {
        super(helper);

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
            _client =
                    new AmazonS3Client(new BasicAWSCredentials(getOmEndpoint().getUsername(),
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
    private final OrchestrationManagerEndpoint _omEndpoint;

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
