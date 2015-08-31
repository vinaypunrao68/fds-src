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

import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.operations.CallChildWorkload;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.GetObjects;

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
        return S3Endpoint.class;
    }
    
    @Override
    protected List<Stream<Operation>> createOperations()
    {
        return Arrays.asList(
                _directories.entrySet()
                            .stream()
                            .flatMap(volumeDirectories -> _createVolumeOperations(volumeDirectories))
                            .toArray(size -> new Stream[size]));
    }
    
    @Override
    protected Stream<Operation> createSetup()
    {
        return Stream.<Operation>concat(
                Stream.of(new CallChildWorkload(new GetVolumes(_volumes::add, getLogOperations()))),
                _volumes.stream().map(v -> new CallChildWorkload(
                        new com.formationds.iodriver.workloads.GetObjects(objName ->
                        {
                            if (objName.endsWith("/"))
                            {
                                Set<String> volumeDirectories = _directories.get(v);
                                if (volumeDirectories == null)
                                {
                                    volumeDirectories = new HashSet<>();
                                    _directories.put(v, volumeDirectories);
                                }
                            }
                        },
                        v,
                        null,
                        null,
                        getLogOperations()))));
    }
    
    private Stream<Operation> _createVolumeOperations(Entry<String, Set<String>> directories)
    {
        String volumeName = directories.getKey();
        Set<String> volumeDirectories = directories.getValue();
        
        Consumer<String> getObjectSetter = obj -> System.out.println(obj);
        
        return volumeDirectories.stream().map(dir -> new GetObjects(volumeName, getObjectSetter));
    }
    
    private final Map<String, Set<String>> _directories;
    
    private final Set<String> _volumes;
}
