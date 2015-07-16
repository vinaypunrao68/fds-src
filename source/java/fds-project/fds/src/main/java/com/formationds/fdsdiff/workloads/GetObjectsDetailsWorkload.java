package com.formationds.fdsdiff.workloads;

import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.FullObjectManifest;
import com.formationds.iodriver.model.ObjectManifest;
import com.formationds.iodriver.operations.GetObject;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.workloads.Workload;

public class GetObjectsDetailsWorkload extends Workload
{
    public GetObjectsDetailsWorkload(String volumeName,
                                     Set<String> objectNames,
                                     Consumer<ObjectManifest> setter,
                                     boolean logOperations)
    {
        super(logOperations);
        
        if (volumeName == null) throw new NullArgumentException("volumeName");
        if (objectNames == null) throw new NullArgumentException("objectNames");
        if (setter == null) throw new NullArgumentException("setter");
        
        _volumeName = volumeName;
        _objectNames = objectNames;
        _setter = setter;
    }
    
    @Override
    public Class<?> getEndpointType()
    {
        return S3Endpoint.class;
    }
    
    @Override
    protected List<Stream<Operation>> createOperations()
    {
        return Collections.singletonList(_objectNames.stream().map(
                objectName -> new GetObject(_volumeName,
                                            objectName,
                                            () -> new FullObjectManifest.Builder<>(),
                                            om -> _setter.accept(om))));
    }

    private final Set<String> _objectNames;
    
    private final Consumer<ObjectManifest> _setter;
    
    private final String _volumeName;
}
