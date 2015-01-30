package com.formationds.iodriver;

import java.net.MalformedURLException;

import com.formationds.commons.FdsConstants;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.reporters.VerificationReporter;
import com.formationds.iodriver.workloads.S3QosTestWorkload;

public final class Config
{
    public final static class Defaults
    {
        public static S3Endpoint getEndpoint()
        {
            return _endpoint;
        }

        public static Logger getLogger()
        {
            return _logger;
        }

        public static VerificationReporter getReporter()
        {
            return _reporter;
        }
        
        public static S3QosTestWorkload getWorkload()
        {
            return _workload;
        }

        static
        {
            Logger newLogger = new ConsoleLogger();
            VerificationReporter newReporter = new VerificationReporter();

            try
            {
                _endpoint =
                        new S3Endpoint(FdsConstants.getS3Endpoint().toString(),
                                       new OrchestrationManagerEndpoint(FdsConstants.Api.getBase(),
                                                                        "admin",
                                                                        "admin",
                                                                        newLogger,
                                                                        true,
                                                                        newReporter),
                                       newLogger,
                                       newReporter);
            }
            catch (MalformedURLException e)
            {
                // Should be impossible.
                throw new IllegalStateException(e);
            }
            _logger = newLogger;
            _reporter = newReporter;
            _workload = new S3QosTestWorkload(4);
        }

        private Defaults()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        private static final S3Endpoint _endpoint;

        private static final Logger _logger;
        
        private static final VerificationReporter _reporter;

        private static final S3QosTestWorkload _workload;
    }

    public S3Endpoint getEndpoint()
    {
        // TODO: Allow this to be specified.
        return Defaults.getEndpoint();
    }

    public VerificationReporter getReporter()
    {
        // TODO: ALlow this to be configured.
        return Defaults.getReporter();
    }
    
    public S3QosTestWorkload getWorkload()
    {
        // TODO: Allow this to be specified.
        return Defaults.getWorkload();
    }
}
