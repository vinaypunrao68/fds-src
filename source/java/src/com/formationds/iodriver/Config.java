package com.formationds.iodriver;

import java.net.MalformedURLException;
import java.util.ArrayList;
import java.util.List;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
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

        static
        {
            Logger newLogger = new ConsoleLogger();
            VerificationReporter newReporter = new VerificationReporter();

            try
            {
                _endpoint =
                        new S3Endpoint(Fds.getS3Endpoint().toString(),
                                       new OrchestrationManagerEndpoint(Fds.Api.getBase(),
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
        }

        private Defaults()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        private static final S3Endpoint _endpoint;

        private static final Logger _logger;

        private static final VerificationReporter _reporter;
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

    public VerificationReporter getReporter()
    {
        // TODO: ALlow this to be configured.
        return Defaults.getReporter();
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
        class IoParams
        {
            public final int max;
            public final int min;

            public IoParams(int min, int max)
            {
                if (min < 1) throw new IllegalArgumentException("min cannot be less than 1.");
                if (max < min) throw new IllegalArgumentException("max cannot be less than min.");

                this.max = max;
                this.min = min;
            }
        }

        // TODO: Allow this to be specified.
        int remainingMin = getSystemIopsMin();
        int remainingMax = getSystemIopsMax();
        int buckets = 4;

        if (remainingMin < buckets)
        {
            throw new ConfigurationException("Min IOPS of " + remainingMin
                                             + " does not allow a guaranteed IOPS to each of the "
                                             + buckets + " buckets.");
        }
        if (remainingMax < remainingMin)
        {
            throw new ConfigurationException("Max IOPS of " + remainingMax
                                             + " is less than min of "
                                             + remainingMin + ", which does not make sense.");
        }
        if (remainingMax < buckets + remainingMin)
        {
            throw new ConfigurationException("Max IOPS of " + remainingMax
                                             + " won't allow at least 1 additional IOPS for each "
                                             + "of " + buckets + " buckets with a min of "
                                             + remainingMin + ".");
        }

        int reservedMin = buckets;
        int reservedMax = reservedMin + buckets;
        remainingMin -= reservedMin;
        remainingMax -= reservedMax;
        
        List<IoParams> bucketParams = new ArrayList<>(buckets);
        for (int i = 0; i != buckets; ++i)
        {
            int myReservedMin = 1;
            int myNonreservedMin = Fds.Random.nextInt(remainingMin);
            int myMin = myReservedMin + myNonreservedMin;
            reservedMin -= myReservedMin;
            remainingMin -= myNonreservedMin;
            
            int myReservedMax = myMin + 1;
            int myNonreservedMax = Fds.Random.nextInt(remainingMax);
            int myMax = myReservedMax + myNonreservedMax;
            reservedMax -= myReservedMax;
            remainingMax -= myReservedMax;
            
            IoParams params = new IoParams(myMin, myMax);
            bucketParams.add(params);
        }
        IoParams lastParams = bucketParams.get(bucketParams.size() - 1);
        lastParams = new IoParams(lastParams.min + remainingMin, lastParams.max + remainingMax);
        bucketParams.set(bucketParams.size() - 1, lastParams);

        return new S3QosTestWorkload(4);
    }

    private final String[] _args;

    private int _disk_iops_max;

    private int _disk_iops_min;

    private Fds.Config _runtimeConfig;
}
