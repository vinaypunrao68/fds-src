package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.net.MalformedURLException;

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint.AuthToken;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.S3Operation;

public final class S3Endpoint extends Endpoint
{
    public S3Endpoint(String s3url, OrchestrationManagerEndpoint omEndpoint, Logger logger) throws MalformedURLException
    {
        if (s3url == null) throw new NullArgumentException("s3url");
        if (omEndpoint == null) throw new NullArgumentException("omEndpoint");
        if (logger == null) throw new NullArgumentException("logger");

        _omEndpoint = omEndpoint;
        _s3url = s3url;
        _logger = logger;
    }

    @Override
    public Endpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        try
        {
            return new S3Endpoint(copyHelper.s3url, copyHelper.omEndpoint, copyHelper.logger);
        }
        catch (MalformedURLException e)
        {
            // This should be impossible.
            throw new RuntimeException(e);
        }
    }

    @Override
    public void exec(Operation operation) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (!(operation instanceof S3Operation))
        {
            throw new IllegalArgumentException("operation must be an instance of "
                                               + S3Operation.class.getName());
        }

        exec((S3Operation)operation);
    }

    public void exec(S3Operation operation) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");

        operation.exec(_client);
    }

    protected final AmazonS3Client getClient() throws IOException
    {
        if (_client == null)
        {
            AuthToken authToken = _omEndpoint.getAuthToken();
            _client =
                    new AmazonS3Client(new BasicAWSCredentials(_omEndpoint.getUsername(),
                                                               authToken.toString()));
            _client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
            _client.setEndpoint(_s3url);
        }
        return _client;
    }

    private final class CopyHelper extends Endpoint.CopyHelper
    {
        public final Logger logger = _logger;
        public final OrchestrationManagerEndpoint omEndpoint = _omEndpoint.copy();
        public final String s3url = _s3url;
    }

    private AmazonS3Client _client;

    private final Logger _logger;

    private final OrchestrationManagerEndpoint _omEndpoint;

    private final String _s3url;
}
