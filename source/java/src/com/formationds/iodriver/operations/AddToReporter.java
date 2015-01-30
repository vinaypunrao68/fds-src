package com.formationds.iodriver.operations;

import java.util.function.Supplier;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.VerificationReporter;
import com.formationds.iodriver.reporters.VerificationReporter.VolumeQosParams;

public class AddToReporter extends S3Operation
{
    public AddToReporter(String bucketName,
                         Supplier<StatBucketVolume.Output> statsGetter)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (statsGetter == null) throw new NullArgumentException("statsGetter");

        _bucketName = bucketName;
        _statsGetter = statsGetter;
    }

    @Override
    public void exec(S3Endpoint endpoint, AmazonS3Client client, VerificationReporter reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");

        StatBucketVolume.Output stats = _statsGetter.get();
        reporter.addVolume(_bucketName, new VolumeQosParams(stats.assured_rate,
                                                            stats.throttle_rate,
                                                            stats.priority));
    }

    private final String _bucketName;

    private final Supplier<StatBucketVolume.Output> _statsGetter;
}
