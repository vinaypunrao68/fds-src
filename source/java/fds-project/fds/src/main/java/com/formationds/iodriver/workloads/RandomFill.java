package com.formationds.iodriver.workloads;

import java.util.Collections;
import java.util.List;
import java.util.UUID;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Stream;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.operations.CreateObject;
import com.formationds.iodriver.operations.CreateVolume;
import com.formationds.iodriver.operations.OrchestrationManagerOperation;
import com.formationds.iodriver.operations.S3Operation;
import com.formationds.iodriver.operations.S3OperationWrapper;

public class RandomFill extends Workload<OrchestrationManagerEndpoint,
                                         OrchestrationManagerOperation>
{
	public RandomFill(int maxVolumes,
                      String pathSeparator,
                      int maxDirectoriesPerLevel,
                      int maxObjectSize,
                      int maxObjectsPerDirectory,
                      int maxDirectoryDepth,
                      boolean logOperations)
	{
        super(OrchestrationManagerEndpoint.class,
              OrchestrationManagerOperation.class,
              logOperations);

		if (pathSeparator == null) throw new NullArgumentException("pathSeparator");

		_maxVolumes = maxVolumes;
		_pathSeparator = pathSeparator;
		_maxDirectoriesPerLevel = maxDirectoriesPerLevel;
		_maxObjectSize = maxObjectSize;
		_maxObjectsPerDirectory = maxObjectsPerDirectory;
		_maxDirectoryDepth = maxDirectoryDepth;
	}

	@Override
	protected List<Stream<OrchestrationManagerOperation>> createOperations()
	{
		return Collections.singletonList(createVolumesOperations());
	}

	private final int _maxDirectoriesPerLevel;

	private final int _maxDirectoryDepth;

	private final int _maxObjectSize;
	
	private final int _maxObjectsPerDirectory;

	private final int _maxVolumes;

	private final String _pathSeparator;

	private Stream<OrchestrationManagerOperation> createVolumesOperations()
	{
		Supplier<String> volumeNames = () -> UUID.randomUUID().toString();
		
		Function<String, Stream<OrchestrationManagerOperation>> createVolumesAndContent =
		        volumeName ->
        		{
        		    Stream<OrchestrationManagerOperation> createVolume =
        		            Stream.of(new CreateVolume(volumeName));

        		    Stream<OrchestrationManagerOperation> createDirectories =
        		            createDirectoriesOperations(volumeName, 0, "");
        		    
        		    return Stream.concat(createVolume, createDirectories);
        		};

		return Stream.generate(volumeNames)
		             .limit(Fds.Random.nextInt(_maxVolumes))
		             .flatMap(createVolumesAndContent);
	}

	private Stream<OrchestrationManagerOperation> createDirectoriesOperations(String volumeName,
	                                                                          int depth,
	                                                                          String parentDir)
	{
	    Stream<OrchestrationManagerOperation> createObjects = createObjectsOperations(volumeName,
	                                                                                  parentDir);
	    
	    if (depth < _maxDirectoryDepth)
	    {
	        return Stream.concat(createObjects,
	                             Stream.generate(() ->
	                                             {
	                                                 return createDirectoriesOperations(volumeName,
	                                                                                    depth + 1,
	                                                                                    parentDir
	                                                                                    + UUID.randomUUID()
	                                                                                          .toString()
                                                                                        + _pathSeparator);
	                                             })
                                       .limit(Fds.Random.nextInt(_maxDirectoriesPerLevel))
	                                   .flatMap(s -> s));
	    }
	    else
	    {
	        return createObjects;
	    }
	}

	private Stream<OrchestrationManagerOperation> createObjectsOperations(String volumeName,
	                                                                      String parentPath)
	{
	    if (volumeName == null) throw new NullArgumentException("volumeName");
	    if (parentPath == null) throw new NullArgumentException("parentPath");
	    
        Stream<S3Operation> createObjects = Stream.generate(
                () ->
                {
                    int thisObjectSize = Fds.Random.nextInt(_maxObjectSize);
                    
                    String fileName = UUID.randomUUID().toString();
                    String fullPath = parentPath + fileName;
                    byte[] content = new byte[thisObjectSize];
                    
                    return new CreateObject(volumeName, fullPath, content);
                });
        
        Stream<OrchestrationManagerOperation> retval =
                createObjects.map(op -> new S3OperationWrapper(op));
        
        return retval.limit(Fds.Random.nextInt(_maxObjectsPerDirectory));
	}
}
