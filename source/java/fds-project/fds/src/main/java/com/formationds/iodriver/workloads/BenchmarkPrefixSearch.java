package com.formationds.iodriver.workloads;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.operations.CallChildWorkload;
import com.formationds.iodriver.operations.GetObjects;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public class BenchmarkPrefixSearch extends Workload
{
    public BenchmarkPrefixSearch(boolean logOperations)
    {
        super(logOperations);

        _directories = new HashMap<>();
        _volumes = new HashSet<>();
    }

    @Override
    public boolean doDryRun()
    {
        return true;
    }

    @Override
    public Class<?> getEndpointType()
    {
        return FdsEndpoint.class;
    }
    
    @Override
    public final void setUp(Endpoint endpoint,
                            AbstractWorkloadEventListener listener) throws ExecutionException
    {
        super.setUp(endpoint, listener);
        
        for (String volumeName : _volumes)
        {
            final String lambdaVolumeName = volumeName;
            
            com.formationds.iodriver.workloads.GetObjects getVolumeObjects =
                    new com.formationds.iodriver.workloads.GetObjects(
                            objName ->
                            {
                                Set<String> volumeDirectories =
                                        _getVolumeDirectories(lambdaVolumeName);

                                int lastSlash = objName.lastIndexOf('/');
                                if (lastSlash > -1)
                                {
                                    volumeDirectories.add(objName.substring(0, lastSlash));
                                }
                            },
                            volumeName,
                            null,
                            null,
                            getLogOperations());
            
            getVolumeObjects.setUp(endpoint, listener);
            getVolumeObjects.runOn(endpoint, listener);
            getVolumeObjects.tearDown(endpoint, listener);
        }
    }
    
    @Override
    protected List<Stream<Operation>> createOperations()
    {
        return Arrays.asList(
                _directories.entrySet()
                            .stream()
                            .map(volumeDirectories -> _createVolumeOperations(volumeDirectories))
                            .toArray(size -> new Stream[size]));
    }
    
    @Override
    protected Stream<Operation> createSetup()
    {
        return Stream.of(new CallChildWorkload(new GetVolumes(_volumes::add, getLogOperations())));
    }
    
    private Stream<Operation> _createVolumeOperations(Entry<String, Set<String>> directories)
    {
        String volumeName = directories.getKey();
        Set<String> volumeDirectories = directories.getValue();
        
        Consumer<String> getObjectSetter = obj -> System.out.println(obj);
        
        return volumeDirectories.stream().map(dir -> new GetObjects(volumeName, getObjectSetter));
    }
    
    private Set<String> _getVolumeDirectories(String volumeName)
    {
        Set<String> retval = _directories.get(volumeName);
        
        if (retval == null)
        {
            retval = new HashSet<>();
            _directories.put(volumeName, retval);
        }
        
        return retval;
    }
    
    private final Map<String, Set<String>> _directories;
    
    private final Set<String> _volumes;
}
