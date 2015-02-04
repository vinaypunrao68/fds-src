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
import com.formationds.iodriver.reporters.WorkflowEventListener;

public final class S3Endpoint extends Endpoint<S3Endpoint, S3Operation>
{
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
                        WorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");

        visit(operation, listener);
    }

    public OrchestrationManagerEndpoint getOmEndpoint()
    {
        return _omEndpoint;
    }

    // @eclipseFormatter:off
    public void visit(S3Operation operation,
                      WorkflowEventListener listener) throws ExecutionException
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

    protected class CopyHelper extends Endpoint<S3Endpoint, S3Operation>.CopyHelper
    {
        public final Logger logger = _logger;
        public final OrchestrationManagerEndpoint omEndpoint = _omEndpoint.copy();
        public final String s3url = _s3url;
    }

    protected S3Endpoint(CopyHelper helper)
    {
        super(helper);

        _logger = helper.logger;
        _omEndpoint = helper.omEndpoint;
        _s3url = helper.s3url;
    }

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

    private AmazonS3Client _client;

    private final Logger _logger;

    private final OrchestrationManagerEndpoint _omEndpoint;

    private final String _s3url;

    static
    {
        System.setProperty(SDKGlobalConfiguration.DISABLE_CERT_CHECKING_SYSTEM_PROPERTY, "true");
    }
}
