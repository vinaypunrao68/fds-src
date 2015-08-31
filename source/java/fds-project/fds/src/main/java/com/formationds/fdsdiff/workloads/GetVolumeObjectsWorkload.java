package com.formationds.fdsdiff.workloads;

import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.operations.GetObjects;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.workloads.Workload;

public class GetVolumeObjectsWorkload extends Workload
{
    public GetVolumeObjectsWorkload(String volumeName,
                                    Consumer<String> setter,
                                    boolean logOperations)
    {
        super(logOperations);
        
        if (volumeName == null) throw new NullArgumentException("volumeName");
        if (setter == null) throw new NullArgumentException("setter");
        
        _volumeName = volumeName;
        _setter = setter;
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
        return Collections.singletonList(Stream.of(new GetObjects(_volumeName, _setter)));
    }
    
    private final String _volumeName;
    
    private final Consumer<String> _setter;
}
