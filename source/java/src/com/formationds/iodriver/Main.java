/**
 * @file
 * @copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.iodriver;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.reporters.ConsoleProgressReporter;
import com.formationds.iodriver.reporters.WorkflowEventListener;
import com.formationds.iodriver.workloads.S3SingleVolumeRateLimitTestWorkload;

/**
 * Entry class for iodriver.
 */
public final class Main
{
    /**
     * Program entry point.
     *
     * @param args The command-line arguments this program was called with.
     */
    public static void main(String[] args)
    {
        int result = Integer.MIN_VALUE;
        try
        {
            if (args == null) throw new NullArgumentException("args");

            Config config = new Config(args);

            WorkflowEventListener listener = config.getListener();
            try (ConsoleProgressReporter reporter =
                    new ConsoleProgressReporter(System.out,
                                                listener.started,
                                                listener.stopped,
                                                listener.volumeAdded))
            {
                Driver<?, ?> driver =
                        new Driver<S3Endpoint,
                        S3SingleVolumeRateLimitTestWorkload>(config.getEndpoint(),
                                                             config.getWorkload(),
                                                             listener,
                                                             config.getValidator());
                driver.runWorkload();
                result = driver.getResult();
            }
        }
        catch (Exception ex)
        {
            LOGGER.logError("Unexpected exception.", ex);
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

    static
    {
        LOGGER = new ConsoleLogger();
    }

    /**
     * This class's logger.
     */
    private static final Logger LOGGER;
}