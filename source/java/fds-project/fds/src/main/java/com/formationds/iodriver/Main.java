/**
 * @file
 * @copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.iodriver;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.S3Operation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
import com.formationds.iodriver.reporters.ConsoleProgressReporter;
import com.formationds.iodriver.reporters.NullWorkflowEventListener;
import com.formationds.iodriver.validators.NullValidator;
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
        Logger logger = null;
        try
        {
            if (args == null) throw new NullArgumentException("args");

            logger = Config.Defaults.getLogger();
            
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
                result = runWorkloadDry(config);
                if (result == 0)
                {
                    result = runWorkload(config);
                }
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

        AbstractWorkflowEventListener listener = config.getListener();
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
            return driver.getResult();
        }
        catch (Exception e)
        {
            throw new ExecutionException("Error executing workload.", e);
        }
    }
    
    private static int runWorkloadDry(Config config) throws ExecutionException
    {
        if (config == null) throw new NullArgumentException("config");
        
        AbstractWorkflowEventListener listener = new NullWorkflowEventListener(config.getLogger());
        try (ConsoleProgressReporter reporter =
                new ConsoleProgressReporter(System.out,
                                            listener.started,
                                            listener.stopped,
                                            listener.volumeAdded))
        {
            S3Workload workload = config.getSelectedWorkload(S3Endpoint.class, S3Operation.class);
            Validator validator = new NullValidator();
            
            Driver<?, ?> driver = new Driver<S3Endpoint, S3Workload>(config.getEndpoint(),
                                                                     workload,
                                                                     listener,
                                                                     validator);
            driver.runWorkload();
            return driver.getResult();
        }
        catch (Exception e)
        {
            throw new ExecutionException("Error executing dry-run of workload.", e);
        }
    }
}
