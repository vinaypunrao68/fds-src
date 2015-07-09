/**
 * @file
 * @copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.iodriver;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
import com.formationds.iodriver.reporters.ConsoleProgressReporter;
import com.formationds.iodriver.reporters.NullWorkflowEventListener;
import com.formationds.iodriver.validators.NullValidator;
import com.formationds.iodriver.validators.Validator;
import com.formationds.iodriver.workloads.Workload;

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
            logger = config.getLogger();

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
                result = runWorkload(config, false);
                if (result == 0)
                {
                    result = runWorkload(config, true);
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

    private static Endpoint<?, ?> getCompatibleEndpoint(Workload<?, ?> workload)
    {
        if (workload == null) throw new NullArgumentException("workload");

        Class<?> neededEndpointClass = workload.getEndpointType();
        if (neededEndpointClass.isAssignableFrom(S3Endpoint.class))
        {
            // TODO: Make this configurable.
            return Config.Defaults.getS3Endpoint();
        }
        else if (neededEndpointClass.isAssignableFrom(OrchestrationManagerEndpoint.class))
        {
            return Config.Defaults.getOMV8Endpoint();
        }
        else
        {
            throw new UnsupportedOperationException(
                    "Cannot find an endpoint of type " + neededEndpointClass.getName() + ".");
        }
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
     * @param validate Whether to evaluate the workload results as part of the return code.
     *
     * @return A system return code.
     *
     * @throws ExecutionException when there is an unexpected error running the workload.
     */
    private static int runWorkload(Config config, boolean validate) throws ExecutionException
    {
        if (config == null) throw new NullArgumentException("config");

        AbstractWorkflowEventListener listener = config.getListener();
        try (ConsoleProgressReporter reporter =
                new ConsoleProgressReporter(System.out,
                                            listener.started,
                                            listener.stopped,
                                            listener.volumeAdded))
        {
            Workload<?, ?> workload = config.getSelectedWorkload();
            Endpoint<?, ?> endpoint = getCompatibleEndpoint(workload);
            Validator validator = validate
                                  ? workload.getSuggestedValidator().orElse(config.getValidator())
                                  : new NullValidator();
            
            Driver<?, ?> driver = Driver.newDriver(endpoint,
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
}
