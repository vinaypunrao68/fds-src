/**
 * @file
 * @copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.iodriver;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.endpoints.OmV7Endpoint;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.ConsoleProgressReporter;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.validators.NullValidator;
import com.formationds.iodriver.validators.Validator;
import com.formationds.iodriver.workloads.BenchmarkPrefixSearchConfig;
import com.formationds.iodriver.workloads.RandomFillConfig;
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
            
            config.addConfig(new RandomFillConfig(config));
            config.addConfig(new BenchmarkPrefixSearchConfig(config));

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

    private static Endpoint getCompatibleEndpoint(Workload workload)
    {
        if (workload == null) throw new NullArgumentException("workload");

        Class<?> neededEndpointClass = workload.getEndpointType();
        if (neededEndpointClass.isAssignableFrom(FdsEndpoint.class))
        {
            return Config.Defaults.getFdsEndpoint();
        }
        else if (neededEndpointClass.isAssignableFrom(S3Endpoint.class))
        {
            // TODO: Make this configurable.
            return Config.Defaults.getS3Endpoint();
        }
        else if (neededEndpointClass.isAssignableFrom(OmV8Endpoint.class))
        {
            return Config.Defaults.getOmV8Endpoint();
        }
        else if (neededEndpointClass.isAssignableFrom(OmV7Endpoint.class))
        {
            return Config.Defaults.getOmV7Endpoint();
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

        try
        {
            Workload workload = config.getSelectedWorkload();
            
            try (WorkloadEventListener listener = new WorkloadEventListener(config.getLogger());
                 ConsoleProgressReporter reporter =
                    new ConsoleProgressReporter(System.out, listener))
            {
                Endpoint endpoint = getCompatibleEndpoint(workload);
                Validator validator = validate
                                      ? workload.getSuggestedValidator().orElse(new NullValidator())
                                      : new NullValidator();
                                      
                Driver driver = Driver.newDriver(endpoint, workload, listener, validator);
                if (validate || workload.doDryRun())
                {
                    driver.runWorkload();
                    return driver.getResult();
                }
                else
                {
                    return 0;
                }
            }
        }
        catch (Exception e)
        {
            throw new ExecutionException("Error executing workload.", e);
        }
    }
}
