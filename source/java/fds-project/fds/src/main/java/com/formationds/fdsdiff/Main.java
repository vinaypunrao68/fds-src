package com.formationds.fdsdiff;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;

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
			
			logger = Config.Defaults.LOGGER;
			
			Config config = new Config(args);
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
				gatherSystemContent(config.getEndpointA(),
						            config.getComparisonDataFormat(),
						            config.getOutputFilename());
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
}
