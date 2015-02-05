package com.formationds.iodriver;

import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.reporters.ConsoleProgressReporter;
import com.formationds.iodriver.reporters.WorkflowEventListener;
import com.formationds.iodriver.workloads.S3QosTestWorkload;

public final class Main
{
    public static void main(String[] args)
    {
        int result = Integer.MIN_VALUE;

        try
        {
            Config config = new Config(args);

            WorkflowEventListener listener = config.getListener();
            try (ConsoleProgressReporter reporter =
                    new ConsoleProgressReporter(System.out,
                                                listener.started,
                                                listener.stopped,
                                                listener.volumeAdded))
            {
                Driver<?, ?> driver =
                        new Driver<S3Endpoint, S3QosTestWorkload>(config.getEndpoint(),
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
        }

        System.exit(result);
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
