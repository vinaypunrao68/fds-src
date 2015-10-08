package com.formationds.iodriver.operations;

import static com.formationds.commons.util.Strings.javaString;

import java.time.Duration;
import java.time.Instant;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.events.PrefixSearch;

public final class GetObjects extends S3Operation
{
    public GetObjects(String bucketName, String prefix, String delimiter, Consumer<String> setter)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (setter == null) throw new NullArgumentException("setter");
        
        _bucketName = bucketName;
        _prefix = prefix;
        _delimiter = delimiter;
        _setter = setter;
    }
    
    public GetObjects(String bucketName, Consumer<String> setter)
    {
        this(bucketName, null, null, setter);
    }
    
    @Override
    public void accept(S3Endpoint endpoint,
                       AmazonS3Client client,
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (context == null) throw new NullArgumentException("context");
        
        Instant start = Instant.now();
        int results = 0;
        ObjectListing objects = null;
        do
        {
            if (objects == null)
            {
                ListObjectsRequest listObjectsRequest = new ListObjectsRequest();
                listObjectsRequest.setBucketName(_bucketName);
                listObjectsRequest.setPrefix(_prefix);
                listObjectsRequest.setDelimiter(_delimiter);
                objects = client.listObjects(listObjectsRequest);
            }
            else
            {
                objects = client.listNextBatchOfObjects(objects);
            }
            
            for (S3ObjectSummary objectSummary : objects.getObjectSummaries())
            {
                ++results;
                _setter.accept(objectSummary.getKey());
            }
        }
        while (objects.isTruncated());
        
        context.sendIfRegistered(
                new PrefixSearch(Instant.now(),
                                 new PrefixSearch.Data(this,
                                                       Duration.between(start, Instant.now()),
                                                       results)));
    }
    
    public String getBucketName()
    {
        return _bucketName;
    }
    
    public String getDelimiter()
    {
        return _delimiter;
    }
    
    public String getPrefix()
    {
        return _prefix;
    }
    
    @Override
    protected Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.of(
                new SimpleImmutableEntry<>("bucketName", javaString(_bucketName)),
                new SimpleImmutableEntry<>("delimiter", javaString(_delimiter)),
                new SimpleImmutableEntry<>("prefix", javaString(_prefix)));
    }
    
    private final String _bucketName;
    
    private final String _delimiter;
    
    private final Consumer<String> _setter;
    
    private final String _prefix;
}
