package com.formationds.iodriver.operations;

import java.util.function.Supplier;

import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.VerificationReporter;

public class AddToReporter extends S3Operation
{
    public AddToReporter(String bucketName,
                         Supplier<VolumeQosSettings> statsGetter)
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

        VolumeQosSettings stats = _statsGetter.get();
        reporter.addVolume(_bucketName, stats);
    }

    private final String _bucketName;

    private final Supplier<VolumeQosSettings> _statsGetter;
}
