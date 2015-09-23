package com.formationds.iodriver.workloads;

import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.fdsdiff.SystemContent.VolumeWrapper;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.operations.ListVolumes;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.BaseWorkloadEventListener;

public class GetVolumes extends Workload
{
    public GetVolumes(Consumer<String> volumeSetter, boolean logOperations)
    {
        super(logOperations);
        
        if (volumeSetter == null) throw new NullArgumentException("volumeSetter");
        
        _volumeSetter = volumeSetter;
    }

    @Override
    public boolean doDryRun()
    {
        return false;
    }

    @Override
    public Class<?> getEndpointType()
    {
        return FdsEndpoint.class;
    }
    
    @Override
    public Class<?> getListenerType()
    {
        return BaseWorkloadEventListener.class;
    }
    
    @Override
    protected List<Stream<Operation>> createOperations()
    {
        Consumer<Collection<VolumeWrapper>> innerVolumeSetter =
                volumes -> volumes.forEach(volume -> _volumeSetter.accept(volume.getVolume()
                                                                                .getName()));
              
        return Collections.singletonList(Stream.of(new ListVolumes(innerVolumeSetter)));
    }
    
    private final Consumer<String> _volumeSetter;
}
