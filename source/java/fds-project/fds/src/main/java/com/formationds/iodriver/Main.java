/**
 * @file
 * @copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.iodriver;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.S3Operation;
import com.formationds.iodriver.reporters.ConsoleProgressReporter;
import com.formationds.iodriver.reporters.WorkflowEventListener;
import com.formationds.iodriver.validators.Validator;
import com.formationds.iodriver.workloads.S3Workload;

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
            if (handleHelp(config))
            {
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
                result = runWorkload(config);
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

    /**
     * Display help if necessary.
     *
     * @param config The runtime config to use.
     *
     * @return Whether help was shown.
     */
    private static boolean handleHelp(Config config)
    {
        if (config == null) throw new NullArgumentException("config");

        if (config.isHelpNeeded())
        {
            config.showHelp();
            return true;
        }
        else
        {
            return false;
        }
    }

    /**
     * Run the requested workload.
     *
     * @param config The runtime config to use.
     *
     * @return A system return code.
     *
     * @throws ExecutionException when there is an unexpected error running the workload.
     */
    private static int runWorkload(Config config) throws ExecutionException
    {
        if (config == null) throw new NullArgumentException("config");

        WorkflowEventListener listener = config.getListener();
        try (ConsoleProgressReporter reporter =
                new ConsoleProgressReporter(System.out,
                                            listener.started,
                                            listener.stopped,
                                            listener.volumeAdded))
        {
            S3Workload workload = config.getSelectedWorkload(S3Endpoint.class, S3Operation.class);
            Validator validator = workload.getSuggestedValidator().orElse(config.getValidator());
            
            Driver<?, ?> driver =
                    new Driver<S3Endpoint, S3Workload>(config.getEndpoint(),
                                                       workload,
                                                       listener,
                                                       validator);
            driver.runWorkload();
            int result = driver.getResult();

            return result;
        }
        catch (Exception e)
        {
            throw new ExecutionException("Error executing workload.", e);
        }
    }
}
