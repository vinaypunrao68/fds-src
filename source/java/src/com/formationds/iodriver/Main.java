package com.formationds.iodriver;

import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;

public final class Main
{
    public static void main(String[] args)
    {
        try
        {
            // TODO: Build config from persistent configuration or command-line options.
            Config config = new Config();
            
            Driver driver = new Driver(config.getEndpoint(), config.getWorkload(), LOGGER);
            driver.runWorkload();
        }
        catch (Exception ex)
        {
            LOGGER.logError("Unexpected exception.", ex);
        }
    }

    private Main()
    {
        throw new UnsupportedOperationException("Trying to instantiate a utility class.");
    }
    
    static
    {
        LOGGER = new ConsoleLogger();
    }
    
    private static final Logger LOGGER;
}
