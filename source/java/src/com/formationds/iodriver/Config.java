package com.formationds.iodriver;

import java.net.MalformedURLException;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.reporters.QosValidator;
import com.formationds.iodriver.reporters.WorkflowEventListener;
import com.formationds.iodriver.workloads.S3QosTestWorkload;

public final class Config
{
    public final static class Defaults
    {
        public static S3Endpoint getEndpoint()
        {
            return _endpoint;
        }

        public static WorkflowEventListener getListener()
        {
            return _listener;
        }
        
        public static Logger getLogger()
        {
            return _logger;
        }

        public static QosValidator getValidator()
        {
            return _validator;
        }

        static
        {
            Logger newLogger = new ConsoleLogger();

            try
            {
                _endpoint =
                        new S3Endpoint(Fds.getS3Endpoint().toString(),
                                       new OrchestrationManagerEndpoint(Fds.Api.getBase(),
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
            _listener = new WorkflowEventListener();
            _logger = newLogger;
            _validator = new QosValidator();
        }

        private Defaults()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        private static final S3Endpoint _endpoint;

        private static final WorkflowEventListener _listener;
        
        private static final Logger _logger;

        private static final QosValidator _validator;
    }

    public Config(String[] args)
    {
        if (args == null) throw new NullArgumentException("args");

        _args = args;
        _disk_iops_max = -1;
        _disk_iops_min = -1;
    }

    public S3Endpoint getEndpoint()
    {
        // TODO: Allow this to be specified.
        return Defaults.getEndpoint();
    }

    public Fds.Config getRuntimeConfig()
    {
        if (_runtimeConfig == null)
        {
            _runtimeConfig = new Fds.Config("iodriver", _args);
        }
        return _runtimeConfig;
    }

    public WorkflowEventListener getListener()
    {
        // TODO: ALlow this to be configured.
        return Defaults.getListener();
    }
    
    public Logger getLogger()
    {
        // TODO: Allow this to be configured.
        return Defaults.getLogger();
    }

    public int getSystemIopsMax() throws ConfigUndefinedException
    {
        if (_disk_iops_max < 0)
        {
            _disk_iops_max = getRuntimeConfig().getIopsMax();
        }
        return _disk_iops_max;
    }

    public int getSystemIopsMin() throws ConfigUndefinedException
    {
        if (_disk_iops_min < 0)
        {
            _disk_iops_min = getRuntimeConfig().getIopsMin();
        }
        return _disk_iops_min;
    }

    public S3QosTestWorkload getWorkload() throws ConfigurationException
    {
        // TODO: Allow # of buckets to be specified.
        int buckets = 4;
        int remainingAssured = getSystemIopsMin();
        final int throttle = getSystemIopsMax();

        if (remainingAssured < buckets)
        {
            throw new ConfigurationException("Min IOPS of " + remainingAssured
                                             + " does not allow a guaranteed IOPS to each of the "
                                             + buckets + " buckets.");
        }
        if (throttle < remainingAssured)
        {
            throw new ConfigurationException("Max IOPS of " + throttle
                                             + " is less than min of "
                                             + remainingAssured + ", which does not make sense.");
        }
        if (throttle < buckets + remainingAssured)
        {
            throw new ConfigurationException("Max IOPS of " + throttle
                                             + " won't allow at least 1 additional IOPS for each "
                                             + "of " + buckets + " buckets with a min of "
                                             + remainingAssured + ".");
        }

        int reservedAssured = buckets;
        remainingAssured -= reservedAssured;

        List<S3QosTestWorkload.IoParams> bucketParams = new ArrayList<>(buckets);
        for (int i = 0; i != buckets; ++i)
        {
            int myAssured;
            if (i == buckets - 1)
            {
                myAssured = reservedAssured + remainingAssured;
            }
            else
            {
                int myReservedAssured = 1;
                int myNonreservedAssured = Fds.Random.nextInt(remainingAssured);
                reservedAssured -= myReservedAssured;
                remainingAssured -= myNonreservedAssured;

                myAssured = myReservedAssured + myNonreservedAssured;
            }
            int myThrottle = Fds.Random.nextInt(myAssured, throttle);

            S3QosTestWorkload.IoParams params =
                    new S3QosTestWorkload.IoParams(myAssured, myThrottle);
            bucketParams.add(params);
        }

        return new S3QosTestWorkload(bucketParams, Duration.ofMinutes(1));
    }
    
    public QosValidator getValidator()
    {
        // TODO: Allow this to be configured.
        return Defaults.getValidator();
    }

    private final String[] _args;

    private int _disk_iops_max;

    private int _disk_iops_min;

    private Fds.Config _runtimeConfig;
}
