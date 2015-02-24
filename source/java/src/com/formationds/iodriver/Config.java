package com.formationds.iodriver;

import java.net.MalformedURLException;
import java.net.URI;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.reporters.WorkflowEventListener;
import com.formationds.iodriver.validators.RateLimitValidator;
import com.formationds.iodriver.validators.Validator;
import com.formationds.iodriver.workloads.S3RateLimitTestWorkload;

/**
 * Global configuration for {@link com.formationds.iodriver}.
 */
public final class Config
{
    /**
     * Default static configuration not pulled from command-line.
     */
    public final static class Defaults
    {
        /**
         * Get the default endpoint.
         * 
         * @return An endpoint to the local system default S3 interface.
         */
        public static S3Endpoint getEndpoint()
        {
            return _endpoint;
        }

        /**
         * Get the default event listener.
         * 
         * @return A stats-gathering and 1-input-many-output event hub.
         */
        public static WorkflowEventListener getListener()
        {
            return _listener;
        }

        /**
         * Get the default logger.
         * 
         * @return A system console logger that writes to standard output.
         */
        public static Logger getLogger()
        {
            return _logger;
        }

        /**
         * Get the default validator.
         *
         * @return A validator that ensures a volumes IOPS did not exceed its configured throttle.
         */
        public static Validator getValidator()
        {
            return _validator;
        }

        static
        {
            Logger newLogger = new ConsoleLogger();

            try
            {
                URI s3Endpoint = Fds.getS3Endpoint();
                String s3EndpointText = s3Endpoint.toString();

                URI apiBase = Fds.Api.getBase();
                OrchestrationManagerEndpoint omEndpoint =
                        new OrchestrationManagerEndpoint(apiBase, "admin", "admin", newLogger, true);

                _endpoint = new S3Endpoint(s3EndpointText, omEndpoint, newLogger);
            }
            catch (MalformedURLException e)
            {
                // Should be impossible.
                throw new IllegalStateException(e);
            }
            _listener = new WorkflowEventListener();
            _logger = newLogger;
            _validator = new RateLimitValidator();
        }

        /**
         * Prevent instantiation.
         */
        private Defaults()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        /**
         * Default endpoint.
         */
        private static final S3Endpoint _endpoint;

        /**
         * Default event listener.
         */
        private static final WorkflowEventListener _listener;

        /**
         * Default logger.
         */
        private static final Logger _logger;

        /**
         * Default validator.
         */
        private static final Validator _validator;
    }

    /**
     * Constructor.
     * 
     * @param args Command-line arguments.
     */
    public Config(String[] args)
    {
        if (args == null) throw new NullArgumentException("args");

        _args = args;
        _disk_iops_max = -1;
        _disk_iops_min = -1;
    }

    /**
     * Get the endpoint to run workloads on.
     * 
     * @return The specified configuration.
     */
    public S3Endpoint getEndpoint()
    {
        // TODO: Allow this to be specified.
        return Defaults.getEndpoint();
    }

    /**
     * Get configuration that is determined dynamically at runtime.
     * 
     * @return The current runtime configuration.
     */
    public Fds.Config getRuntimeConfig()
    {
        if (_runtimeConfig == null)
        {
            _runtimeConfig = new Fds.Config("iodriver", _args);
        }
        return _runtimeConfig;
    }

    /**
     * Get the configured listener.
     * 
     * @return An event hub.
     */
    public WorkflowEventListener getListener()
    {
        // TODO: ALlow this to be configured.
        return Defaults.getListener();
    }

    /**
     * Get the configured logger.
     * 
     * @return A logger.
     */
    public Logger getLogger()
    {
        // TODO: Allow this to be configured.
        return Defaults.getLogger();
    }

    /**
     * Get the maximum IOPS this system will allow.
     * 
     * @return A number of IOPS.
     * 
     * @throws ConfigUndefinedException when {@value Fds.Config#DISK_IOPS_MAX_CONFIG} is not defined
     *             in platform.conf.
     */
    public int getSystemIopsMax() throws ConfigUndefinedException
    {
        if (_disk_iops_max < 0)
        {
            _disk_iops_max = getRuntimeConfig().getIopsMax();
        }
        return _disk_iops_max;
    }

    /**
     * Get the guaranteed IOPS this system can support.
     * 
     * @return A number of IOPS.
     * 
     * @throws ConfigUndefinedException when {@value Fds.Config#DISK_IOPS_MIN_CONFIG} is not defined
     *             in platform.conf.
     */
    public int getSystemIopsMin() throws ConfigUndefinedException
    {
        if (_disk_iops_min < 0)
        {
            _disk_iops_min = getRuntimeConfig().getIopsMin();
        }
        return _disk_iops_min;
    }

    /**
     * Get the configured workload.
     * 
     * @return A workload to run.
     * 
     * @throws ConfigurationException when the system assured and throttle IOPS rates are not within
     *             a testable range.
     */
    public S3RateLimitTestWorkload getWorkload() throws ConfigurationException
    {
        final int systemAssured = getSystemIopsMin();
        final int systemThrottle = getSystemIopsMax();
        final int headroomNeeded = 50;

        if (systemAssured <= 0)
        {
            throw new ConfigurationException("System-wide assured rate of " + systemAssured
                                             + " does not result in a sane configuration.");
        }
        if (systemAssured > systemThrottle - headroomNeeded)
        {
            throw new ConfigurationException("System-wide throttle of " + systemThrottle
                                             + " leaves less than " + headroomNeeded
                                             + " IOPS of headroom over system assured "
                                             + systemAssured + " IOPS.");
        }

        return new S3RateLimitTestWorkload(systemAssured + headroomNeeded);
    }

    /**
     * Get the configured validator.
     * 
     * @return A validator.
     */
    public Validator getValidator()
    {
        // TODO: Allow this to be configured.
        return Defaults.getValidator();
    }

    /**
     * Command-line arguments.
     */
    private final String[] _args;

    /**
     * Maximum IOPS allowed by the system.
     */
    private int _disk_iops_max;

    /**
     * Minimum IOPS guaranteed by the system.
     */
    private int _disk_iops_min;

    /**
     * Runtime configuration.
     */
    private Fds.Config _runtimeConfig;
}
