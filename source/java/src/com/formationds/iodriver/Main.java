package com.formationds.iodriver;

import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.workloads.S3QosTestWorkload;

public final class Main
{
    public static void main(String[] args)
    {
        try
        {
            Config config = new Config(args);

            Driver<?, ?> driver =
                    new Driver<S3Endpoint, S3QosTestWorkload>(config.getEndpoint(),
                                                              config.getWorkload(),
                                                              config.getReporter());
            driver.runWorkload();
            
            System.exit(driver.getResult());
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
