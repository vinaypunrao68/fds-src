package com.formationds.iodriver.operations;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

import com.amazonaws.AmazonClientException;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.formationds.iodriver.NullArgumentException;

public class CreateObject extends S3Operation
{
    public CreateObject(String bucketName, String key, String content)
    {
        this(bucketName, key, getBytes(content));
    }

    public CreateObject(String bucketName, String key, byte[] content)
    {
        this(bucketName, key, toInputStream(content), getLength(content));
    }

    public CreateObject(String bucketName, String key, InputStream input, long contentLength)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (key == null) throw new NullArgumentException("key");
        if (input == null) throw new NullArgumentException("input");
        if (contentLength < 0) throw new IllegalArgumentException("contentLength must be >= 0.");

        _bucketName = bucketName;
        _contentLength = contentLength;
        _key = key;
        _input = input;
    }

    @Override
    public void exec(AmazonS3Client client) throws ExecutionException
    {
        if (client == null) throw new NullArgumentException("client");

        ObjectMetadata metadata = new ObjectMetadata();
        metadata.setContentLength(_contentLength);
        try
        {
            client.putObject(_bucketName, _key, _input, metadata);
        }
        catch (AmazonClientException e)
        {
            throw new ExecutionException("Error creating file.", e);
        }
        finally
        {
            try
            {
                _input.close();
            }
            catch (IOException e)
            {
                throw new ExecutionException("Error closing input stream.", e);
            }
        }
    }

    private final String _bucketName;

    private final long _contentLength;

    private InputStream _input;

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
