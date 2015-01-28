package com.formationds.iodriver;

import java.net.MalformedURLException;

import com.formationds.commons.FdsConstants;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.workloads.S3QosTestWorkload;

public final class Config
{
    public final static class Defaults
    {
        public static Endpoint getEndpoint()
        {
            return _endpoint;
        }

        public static Logger getLogger()
        {
            return _logger;
        }

        public static Workload getWorkload()
        {
            return _workload;
        }

        static
        {
            Logger newLogger = new ConsoleLogger();

            try
            {
                _endpoint =
                        new S3Endpoint(FdsConstants.getS3Endpoint().toString(),
                                       new OrchestrationManagerEndpoint(FdsConstants.getApiPath(),
                                                                        "admin",
                                                                        "admin",
                                                                        newLogger,
                                                                        true),
                                       newLogger);
            }
            catch (MalformedURLException e)
            {
                // Should be impossible.
                throw new IllegalStateException(e);
            }
            _logger = newLogger;
            _workload = new S3QosTestWorkload(4);
        }

        private Defaults()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        private static final Endpoint _endpoint;

        private static final Logger _logger;

        private static final Workload _workload;
    }

    public Endpoint getEndpoint()
    {
        // TODO: Allow this to be specified.
        return Defaults.getEndpoint();
    }

    public Workload getWorkload()
    {
        // TODO: Allow this to be specified.
        return Defaults.getWorkload();
    }
}
