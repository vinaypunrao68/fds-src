package com.formationds.iodriver.operations;

import static com.formationds.commons.util.Strings.javaString;

import java.io.IOException;
import java.io.InputStream;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.Map;
import java.util.function.Consumer;

import javax.xml.bind.DatatypeConverter;

import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public final class GetObject extends S3Operation
{
    public GetObject(String bucketName,
                     String key,
                     Consumer<Map<String, String>> metadataSetter,
                     Consumer<InputStream> contentSetter,
                     Consumer<byte[]> md5SumSetter,
                     Consumer<ZonedDateTime> lastModifiedSetter,
                     Consumer<Long> sizeSetter)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (key == null) throw new NullArgumentException("key");
        
        _bucketName = bucketName;
        _key = key;
        _metadataSetter = metadataSetter;
        _contentSetter = contentSetter;
        _md5SumSetter = md5SumSetter;
        _lastModifiedSetter = lastModifiedSetter;
        _sizeSetter = sizeSetter;
    }
    
    @Override
    public void accept(S3Endpoint endpoint,
                       AmazonS3Client client,
                       AbstractWorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (listener == null) throw new NullArgumentException("listener");

        try (S3Object object = client.getObject(_bucketName, _key))
        {
            {
                ObjectMetadata metadata = object.getObjectMetadata();
                
                if (_metadataSetter != null)
                {
                    _metadataSetter.accept(metadata.getUserMetadata());
                }
                
                if (_md5SumSetter != null)
                {
                    String md5String = metadata.getContentMD5();
                    byte[] md5 = DatatypeConverter.parseBase64Binary(md5String);
                    _md5SumSetter.accept(md5);
                }
                
                if (_lastModifiedSetter != null)
                {
                    ZonedDateTime lastModified = metadata.getLastModified()
                                                         .toInstant()
                                                         .atZone(_PACIFIC);
                    _lastModifiedSetter.accept(lastModified);
                }
                
                if (_sizeSetter != null)
                {
                    _sizeSetter.accept(metadata.getContentLength());
                }
            }
            
            if (_contentSetter != null)
            {
                try (S3ObjectInputStream contentStream = object.getObjectContent())
                {
                    _contentSetter.accept(contentStream);
                }
                catch (IOException e)
                {
                    throw new ExecutionException("Error closing content stream for "
                                                 + javaString(_bucketName + ":" + _key) + ".",
                                                 e);
                }
            }
        }
        catch (IOException e)
        {
            throw new ExecutionException("Error closing object "
                                         + javaString(_bucketName + ":" + _key) + ".",
                                         e);
        }
    }
    
    static
    {
        _PACIFIC = ZoneId.of("America/Los_Angeles");
    }
    
    private final String _bucketName;
    
    private final Consumer<InputStream> _contentSetter;
    
    private final String _key;
    
    private final Consumer<ZonedDateTime> _lastModifiedSetter;
    
    private final Consumer<Map<String, String>> _metadataSetter;
    
    private final Consumer<byte[]> _md5SumSetter;
    
    private final Consumer<Long> _sizeSetter;
    
    private final static ZoneId _PACIFIC;
}
