package com.formationds.iodriver.workloads;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Optional;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.Driver;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.operations.CallChildWorkload;
import com.formationds.iodriver.operations.GetObjects;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.BenchmarkPrefixSearchReporter;
import com.formationds.iodriver.validators.NullValidator;

public class BenchmarkPrefixSearch extends Workload
{
    public BenchmarkPrefixSearch()
    {
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
    public Optional<Closeable> getSuggestedReporter(PrintStream output,
                                                    WorkloadContext context)
    {
        if (output == null) throw new NullArgumentException("output");
        if (context == null) throw new NullArgumentException("context");
        
        return Optional.of(new BenchmarkPrefixSearchReporter(output, context));
    }

    @Override
    public BenchmarkPrefixSearchContext newContext(Logger logger)
    {
        if (logger == null) throw new NullArgumentException("logger");
        
        return new BenchmarkPrefixSearchContext(logger);
    }
    
    @Override
    public final void setUp(Endpoint endpoint,
                            WorkloadContext context) throws ExecutionException
    {
        super.setUp(endpoint, context);
        
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
                                    volumeDirectories.add(objName.substring(0, lastSlash + 1));
                                }
                            },
                            volumeName,
                            null,
                            null);
            
            try (WorkloadContext innerContext = getVolumeObjects.newContext(context.getLogger()))
            {
                Driver driver = Driver.newDriver(endpoint,
                                                 getVolumeObjects,
                                                 getVolumeObjects.getSuggestedValidator()
                                                                 .orElse(new NullValidator()));
                driver.runWorkload(innerContext);
                int result = driver.getResult(innerContext);
                if (result != 0)
                {
                    throw new ExecutionException("Getting volume objects failed with code " + result);
                }
            }
            catch (IOException e)
            {
                throw new ExecutionException("Error closing inner context.", e);
            }
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
        return Stream.of(new CallChildWorkload(new GetVolumes(_volumes::add)));
    }
    
    private Stream<Operation> _createVolumeOperations(Entry<String, Set<String>> directories)
    {
        String volumeName = directories.getKey();
        Set<String> volumeDirectories = directories.getValue();

        Consumer<String> getObjectSetter = obj -> { };
        
        return volumeDirectories.stream()
                                .map(dir -> new GetObjects(volumeName, dir, "/", getObjectSetter));
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
