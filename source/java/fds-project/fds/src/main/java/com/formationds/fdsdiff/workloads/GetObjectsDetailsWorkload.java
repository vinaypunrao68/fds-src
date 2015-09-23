package com.formationds.fdsdiff.workloads;

import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Supplier;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.ObjectManifest;
import com.formationds.iodriver.operations.GetObject;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.BaseWorkloadEventListener;
import com.formationds.iodriver.workloads.Workload;

public final class GetObjectsDetailsWorkload extends Workload
{
    public GetObjectsDetailsWorkload(
            String volumeName,
            Set<String> objectNames,
            Supplier<ObjectManifest.Builder<?, ? extends ObjectManifest>> builderSupplier,
            Consumer<ObjectManifest> setter,
            boolean logOperations)
    {
        super(logOperations);
        
        if (volumeName == null) throw new NullArgumentException("volumeName");
        if (objectNames == null) throw new NullArgumentException("objectNames");
        if (builderSupplier == null) throw new NullArgumentException("builderSupplier");
        if (setter == null) throw new NullArgumentException("setter");
        
        _volumeName = volumeName;
        _objectNames = objectNames;
        _builderSupplier = builderSupplier;
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
    public Class<?> getListenerType()
    {
        return BaseWorkloadEventListener.class;
    }
    
    @Override
    protected List<Stream<Operation>> createOperations()
    {
        return Collections.singletonList(_objectNames.stream().map(
                objectName -> new GetObject(_volumeName,
                                            objectName,
                                            _builderSupplier,
                                            _setter)));
    }

    private final Supplier<ObjectManifest.Builder<?, ? extends ObjectManifest>> _builderSupplier;
    
    private final Set<String> _objectNames;
    
    private final Consumer<ObjectManifest> _setter;
    
    private final String _volumeName;
}
