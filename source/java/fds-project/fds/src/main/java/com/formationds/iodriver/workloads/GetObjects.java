package com.formationds.iodriver.workloads;

import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.operations.Operation;

public class GetObjects extends Workload
{
    public GetObjects(Consumer<String> objectSetter,
                      String bucketName,
                      String prefix,
                      String delimiter)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (objectSetter == null) throw new NullArgumentException("objectSetter");
        
        _bucketName = bucketName;
        _delimiter = delimiter;
        _objectSetter = objectSetter;
        _prefix = prefix;
    }
    
    @Override
    public boolean doDryRun()
    {
        return false;
    }
    
    @Override
    public Class<?> getEndpointType()
    {
        return S3Endpoint.class;
    }
    
    @Override
    protected List<Stream<Operation>> createOperations()
    {
        return Collections.singletonList(Stream.of(
                new com.formationds.iodriver.operations.GetObjects(_bucketName,
                                                                   _prefix,
                                                                   _delimiter,
                                                                   _objectSetter)));
    }
    
    private final String _bucketName;
    
    private final String _delimiter;
    
    private final Consumer<String> _objectSetter;
    
    private final String _prefix;
}
