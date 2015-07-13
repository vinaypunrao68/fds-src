package com.formationds.iodriver.workloads;

import java.util.Collections;
import java.util.List;
import java.util.UUID;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Stream;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.operations.CreateObject;
import com.formationds.iodriver.operations.CreateVolume;
import com.formationds.iodriver.operations.FdsOmOperationWrapper;
import com.formationds.iodriver.operations.FdsOperation;
import com.formationds.iodriver.operations.FdsS3OperationWrapper;

public class RandomFill extends Workload<FdsEndpoint, FdsOperation>
{
	public RandomFill(int maxVolumes,
                      String pathSeparator,
                      int maxDirectoriesPerLevel,
                      int maxObjectSize,
                      int maxObjectsPerDirectory,
                      int maxDirectoryDepth,
                      boolean logOperations)
	{
        super(FdsEndpoint.class, FdsOperation.class, logOperations);

		if (pathSeparator == null) throw new NullArgumentException("pathSeparator");

		_maxVolumes = maxVolumes;
		_pathSeparator = pathSeparator;
		_maxDirectoriesPerLevel = maxDirectoriesPerLevel;
		_maxObjectSize = maxObjectSize;
		_maxObjectsPerDirectory = maxObjectsPerDirectory;
		_maxDirectoryDepth = maxDirectoryDepth;
	}

	@Override
	protected List<Stream<FdsOperation>> createOperations()
	{
		return Collections.singletonList(createVolumesOperations());
	}

	private final int _maxDirectoriesPerLevel;

	private final int _maxDirectoryDepth;

	private final int _maxObjectSize;
	
	private final int _maxObjectsPerDirectory;

	private final int _maxVolumes;

	private final String _pathSeparator;

	private Stream<FdsOperation> createVolumesOperations()
	{
		Supplier<String> volumeNames = () -> UUID.randomUUID().toString();
		
		Function<String, Stream<FdsOperation>> createVolumesAndContent =
		        volumeName ->
        		{
        		    Stream<FdsOperation> createVolume =
        		            Stream.of(new FdsOmOperationWrapper(new CreateVolume(volumeName)));

        		    Stream<FdsOperation> createDirectories =
        		            createDirectoriesOperations(volumeName, 0, "");
        		    
        		    return Stream.concat(createVolume, createDirectories);
        		};

		return Stream.generate(volumeNames)
		             .limit(Fds.Random.nextInt(1, _maxVolumes))
		             .flatMap(createVolumesAndContent);
	}

	private Stream<FdsOperation> createDirectoriesOperations(String volumeName,
	                                                         int depth,
	                                                         String parentDir)
	{
	    Stream<FdsOperation> retval = Stream.empty();

	    if (!parentDir.isEmpty())
	    {
	        retval = Stream.concat(retval, Stream.of(
	                new FdsS3OperationWrapper(new CreateObject(volumeName,
	                                                           parentDir,
	                                                           "",
	                                                           false))));
	    }

	    retval = Stream.concat(retval, createObjectsOperations(volumeName, parentDir));

	    if (depth < _maxDirectoryDepth)
	    {
	        retval = Stream.concat(retval, Stream.generate(
                    () ->
                    {
                        return createDirectoriesOperations(volumeName,
                                                           depth + 1,
                                                           parentDir
                                                           + UUID.randomUUID()
                                                                 .toString()
                                                           + _pathSeparator);
                    }).limit(Fds.Random.nextInt(_maxDirectoriesPerLevel))
	                  .flatMap(s -> s));
	    }

	    return retval;
	}

	private Stream<FdsOperation> createObjectsOperations(String volumeName,
	                                                                      String parentPath)
	{
	    if (volumeName == null) throw new NullArgumentException("volumeName");
	    if (parentPath == null) throw new NullArgumentException("parentPath");
	    
        Stream<FdsOperation> createObjects = Stream.generate(
                () ->
                {
                    int thisObjectSize = Fds.Random.nextInt(_maxObjectSize);
                    
                    String fileName = UUID.randomUUID().toString();
                    String fullPath = parentPath + fileName;
                    byte[] content = new byte[thisObjectSize];
                    
                    return new FdsS3OperationWrapper(
                            new CreateObject(volumeName, fullPath, content, false));
                });
        
        return createObjects.limit(Fds.Random.nextInt(_maxObjectsPerDirectory));
	}
}
