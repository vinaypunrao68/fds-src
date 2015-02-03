package com.formationds.iodriver.operations;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.function.Supplier;

import com.amazonaws.AmazonClientException;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.VerificationReporter;

public final class CreateObject extends S3Operation
{
    public CreateObject(String bucketName, String key, String content)
    {
        this(bucketName, key, content, true);
    }
    
    public CreateObject(String bucketName, String key, String content, boolean doReporting)
    {
        this(bucketName, key, getBytes(content), doReporting);
    }

    public CreateObject(String bucketName, String key, byte[] content, boolean doReporting)
    {
        this(bucketName, key, () -> toInputStream(content), getLength(content), doReporting);
    }

    public CreateObject(String bucketName,
                        String key,
                        Supplier<InputStream> input,
                        long contentLength,
                        boolean doReporting)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (key == null) throw new NullArgumentException("key");
        if (input == null) throw new NullArgumentException("input");
        if (contentLength < 0) throw new IllegalArgumentException("contentLength must be >= 0.");

        _bucketName = bucketName;
        _contentLength = contentLength;
        _key = key;
        _input = input;
        _doReporting = doReporting;
    }

    @Override
    public void exec(S3Endpoint endpoint, AmazonS3Client client, VerificationReporter reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");

        ObjectMetadata metadata = new ObjectMetadata();
        metadata.setContentLength(_contentLength);
        try (InputStream input = _input.get())
        {
            client.putObject(_bucketName, _key, input, metadata);
        }
        catch (AmazonClientException e)
        {
            throw new ExecutionException("Error creating file.", e);
        }
        catch (IOException e)
        {
            throw new ExecutionException("Error closing input stream.", e);
        }
        
        if (_doReporting)
        {
            reporter.reportIo(_bucketName);
        }
    }

    private final String _bucketName;

    private final long _contentLength;

    private final boolean _doReporting;
    
    private Supplier<InputStream> _input;

    private final String _key;

    private static final int getLength(byte[] content)
    {
        if (content == null) throw new NullArgumentException("content");

        return content.length;
    }

    private static final byte[] getBytes(String content)
    {
        if (content == null) throw new NullArgumentException("content");

        return content.getBytes(StandardCharsets.UTF_8);
    }

    private static final ByteArrayInputStream toInputStream(byte[] content)
    {
        if (content == null) throw new NullArgumentException("content");

        return new ByteArrayInputStream(content);
    }
}
