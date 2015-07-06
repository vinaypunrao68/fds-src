package com.formationds.fdsdiff;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;

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
				// TODO: Full implementation.
				// FIXME: Can't just call .get() like that.
				result = gatherSystemContent(config.getEndpointA(),
						                     config.getComparisonDataFormat(),
						                     config.getOutputFilename().get());
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
	
	private static int gatherSystemContent(OrchestrationManagerEndpoint endpoint,
										   ComparisonDataFormat format,
										   OutputStream output)
    {
		if (endpoint == null) throw new NullArgumentException("endpoint");
		if (output == null) throw new NullArgumentException("output");
		
		
		// FIXME: We'll get here for failure too.
		// Success!
		return 0;
	}

	private static int gatherSystemContent(OrchestrationManagerEndpoint endpoint,
										   ComparisonDataFormat format,
										   String outputFilename) throws FileNotFoundException,
										                                 IOException
    {
		if (endpoint == null) throw new NullArgumentException("endpoint");
		if (outputFilename == null) throw new NullArgumentException("outputFilename");
		
		if (outputFilename.equals("-"))
		{
			return gatherSystemContent(endpoint, format, Config.Console.getStdout());
		}
		
		try (FileOutputStream os = new FileOutputStream(outputFilename))
		{
			return gatherSystemContent(endpoint, format, os);
		}
    }
}
