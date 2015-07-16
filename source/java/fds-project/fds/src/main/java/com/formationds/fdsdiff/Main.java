package com.formationds.fdsdiff;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.function.Consumer;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.fdsdiff.workloads.GetObjectsDetailsWorkload;
import com.formationds.fdsdiff.workloads.GetSystemConfigWorkload;
import com.formationds.fdsdiff.workloads.GetVolumeObjectsWorkload;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.model.ObjectManifest;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;
import com.formationds.iodriver.reporters.NullWorkloadEventListener;

/**
 * Entry class for fdsdiff.
 */
public final class Main
{
	/**
	 * Program entry point.
	 * 
	 * @param args Command-line arguments.
	 */
	public static void main(String[] args)
	{
		int result = Integer.MIN_VALUE;
		Logger logger = null;
		try
		{
			if (args == null) throw new NullArgumentException("args");
			
			logger = Config.Defaults.getLogger();
			
			Config config = new Config(args);
			logger = config.getLogger();
			
			if (config.isHelpNeeded())
			{
				config.showHelp();
				if (config.isHelpExplicitlyRequested())
				{
					result = 0;
				}
				else
				{
					result = 2;
				}
			}
			else
			{
			    ComparisonDataFormat format = config.getComparisonDataFormat();
			    
				// TODO: Full implementation.
				result = gatherSystemContent(_getSystemContentContainer(format),
				                             config.getEndpointA(),
						                     format,
						                     config.getOutputFilename(),
						                     logger);
			}
		}
		catch (Exception ex)
		{
			if (logger != null)
			{
				logger.logError("Unexpected exception.", ex);
			}
			result = 1;
		}
		
		System.exit(result);
	}

	/**
	 * Prevent instantiation.
	 */
	private Main()
	{
		throw new UnsupportedOperationException("Trying to instantiate a utility class.");
	}
	
	private static int gatherSystemContent(SystemContent contentContainer,
	                                       FdsEndpoint endpoint,
										   ComparisonDataFormat format,
										   OutputStream output,
										   Logger logger) throws ExecutionException
    {
	    if (contentContainer == null) throw new NullArgumentException("contentContainer");
		if (endpoint == null) throw new NullArgumentException("endpoint");
		if (output == null) throw new NullArgumentException("output");
		if (logger == null) throw new NullArgumentException("logger");
		
		GetSystemConfigWorkload getSystemConfig = new GetSystemConfigWorkload(contentContainer,
		                                                                      true);
		
		// TODO: Add a real listener here. Probably after refactoring the listener to be not so
		//       specific to QoS test workloads.
		AbstractWorkloadEventListener listener = new NullWorkloadEventListener(logger);
		
		getSystemConfig.runOn(endpoint, listener);
		
		for (Volume volume : contentContainer.getVolumes())
		{
		    String volumeName = volume.getName();
		    
		    Consumer<String> objectNameSetter =
		            contentContainer.getVolumeObjectNameAdder(volumeName);
		    GetVolumeObjectsWorkload getVolumeObjects =
		            new GetVolumeObjectsWorkload(volumeName, objectNameSetter, true);
		    getVolumeObjects.runOn(endpoint, listener);

		    Consumer<ObjectManifest> objectDetailSetter =
		            contentContainer.getVolumeObjectAdder(volumeName);
		    GetObjectsDetailsWorkload getObjectsDetails =
		            new GetObjectsDetailsWorkload(volumeName,
		                                          contentContainer.getObjectNames(volumeName),
		                                          objectDetailSetter,
		                                          true);
		    getObjectsDetails.runOn(endpoint, listener);
		}
		
		System.out.println(getSystemConfig.getContentContainer().toString());
		
		// FIXME: We'll get here for failure too.
		// Success!
		return 0;
	}

	private static int gatherSystemContent(SystemContent contentContainer,
	                                       FdsEndpoint endpoint,
										   ComparisonDataFormat format,
										   String outputFilename,
										   Logger logger) throws FileNotFoundException,
										                         IOException,
										                         ExecutionException
    {
	    if (contentContainer == null) throw new NullArgumentException("contentContainer");
		if (endpoint == null) throw new NullArgumentException("endpoint");
		if (outputFilename == null) throw new NullArgumentException("outputFilename");
		if (logger == null) throw new NullArgumentException("logger");
		
		if (outputFilename.equals("-"))
		{
			return gatherSystemContent(contentContainer,
			                           endpoint,
			                           format,
			                           Config.Console.getStdout(),
			                           logger);
		}
		
		try (FileOutputStream os = new FileOutputStream(outputFilename))
		{
			return gatherSystemContent(contentContainer, endpoint, format, os, logger);
		}
    }
	
	private static final SystemContent _getSystemContentContainer(ComparisonDataFormat format)
	{
	    switch (format)
	    {
	    case FULL:
	        return new FullSystemContent();
        default:
            throw new IllegalArgumentException("format: " + format + " not recognized.");
	    }
	}
}
