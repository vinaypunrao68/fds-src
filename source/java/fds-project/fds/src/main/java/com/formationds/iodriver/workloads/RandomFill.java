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
import com.formationds.iodriver.operations.Operation;

public class RandomFill extends Workload
{
	public RandomFill(int maxVolumes,
                      String pathSeparator,
                      int maxDirectoriesPerLevel,
                      int maxObjectSize,
                      int maxObjectsPerDirectory,
                      int maxDirectoryDepth,
                      boolean logOperations)
	{
        super(logOperations);

		if (pathSeparator == null) throw new NullArgumentException("pathSeparator");

		_maxVolumes = maxVolumes;
		_pathSeparator = pathSeparator;
		_maxDirectoriesPerLevel = maxDirectoriesPerLevel;
		_maxObjectSize = maxObjectSize;
		_maxObjectsPerDirectory = maxObjectsPerDirectory;
		_maxDirectoryDepth = maxDirectoryDepth;
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
	protected List<Stream<Operation>> createOperations()
	{
		return Collections.singletonList(createVolumesOperations());
	}

	private final int _maxDirectoriesPerLevel;

	private final int _maxDirectoryDepth;

	private final int _maxObjectSize;
	
	private final int _maxObjectsPerDirectory;

	private final int _maxVolumes;

	private final String _pathSeparator;

	private Stream<Operation> createVolumesOperations()
	{
		Supplier<String> volumeNames = () -> UUID.randomUUID().toString();
		
		Function<String, Stream<Operation>> createVolumesAndContent =
		        volumeName ->
        		{
        		    Stream<Operation> createVolume =
        		            Stream.of(new CreateVolume(volumeName));

        		    Stream<Operation> createDirectories =
        		            createDirectoriesOperations(volumeName, 0, "");
        		    
        		    return Stream.concat(createVolume, createDirectories);
        		};

		return Stream.generate(volumeNames)
		             .limit(Fds.Random.nextInt(1, _maxVolumes))
		             .flatMap(createVolumesAndContent);
	}

	private Stream<Operation> createDirectoriesOperations(String volumeName,
	                                                         int depth,
	                                                         String parentDir)
	{
	    Stream<Operation> retval = Stream.empty();

	    if (!parentDir.isEmpty())
	    {
	        retval = Stream.concat(retval, Stream.of(new CreateObject(volumeName,
	                                                                  parentDir,
	                                                                  "",
	                                                                  false)));
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

	private Stream<Operation> createObjectsOperations(String volumeName, String parentPath)
	{
	    if (volumeName == null) throw new NullArgumentException("volumeName");
	    if (parentPath == null) throw new NullArgumentException("parentPath");
	    
        Stream<Operation> createObjects = Stream.generate(
                () ->
                {
                    int thisObjectSize = Fds.Random.nextInt(_maxObjectSize);
                    
                    String fileName = UUID.randomUUID().toString();
                    String fullPath = parentPath + fileName;
                    byte[] content = new byte[thisObjectSize];
                    Fds.Random.nextBytes(content);
                    
                    return new CreateObject(volumeName, fullPath, content, false);
                });
        
        return createObjects.limit(Fds.Random.nextInt(_maxObjectsPerDirectory));
	}
}
