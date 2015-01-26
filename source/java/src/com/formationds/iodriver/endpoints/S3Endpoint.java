package com.formationds.iodriver.endpoints;

import java.net.MalformedURLException;

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.formationds.iodriver.NotImplementedException;
import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.S3Operation;

public final class S3Endpoint extends Endpoint
{
    public S3Endpoint(String endpoint, String accessKey, String secretKey, Logger logger) throws MalformedURLException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (accessKey == null) throw new NullArgumentException("accessKey");
        if (secretKey == null) throw new NullArgumentException("secretKey");
        if (logger == null) throw new NullArgumentException("logger");

        _client = new AmazonS3Client(new BasicAWSCredentials(accessKey, secretKey));
        _client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        _client.setEndpoint(endpoint);

        _accessKey = accessKey;
        _endpoint = endpoint;
        _logger = logger;
        _secretKey = secretKey;
    }

    @Override
    public Endpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        try
        {
            return new S3Endpoint(copyHelper.endpoint,
                                  copyHelper.accessKey,
                                  copyHelper.secretKey,
                                  copyHelper.logger);
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

    private final class CopyHelper extends Endpoint.CopyHelper
    {
        public final String accessKey = _accessKey;
        public final String endpoint = _endpoint;
        public final Logger logger = _logger;
        public final String secretKey = _secretKey;
    }

    private final String _accessKey;

    private final AmazonS3Client _client;

    private final String _endpoint;

    private final Logger _logger;

    private final String _secretKey;
}
