package com.formationds.iodriver.operations;

import java.util.function.Consumer;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.apis.MediaPolicy;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.VerificationReporter;

public final class StatBucketVolume extends S3Operation
{
    public static final class Output extends StatVolume.Output
    {
        public Output(int assured_rate,
                      int throttle_rate,
                      int priority,
                      long commit_log_retention,
                      MediaPolicy mediaPolicy,
                      long id)
        {
            super(assured_rate, throttle_rate, priority, commit_log_retention, mediaPolicy, id);
        }
    }

    public StatBucketVolume(String bucketName, Consumer<Output> consumer)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (consumer == null) throw new NullArgumentException("consumer");

        _statVolumeOp =
                new StatVolume(bucketName,
                               output -> consumer.accept(new Output(output.assured_rate,
                                                                    output.throttle_rate,
                                                                    output.priority,
                                                                    output.commit_log_retention,
                                                                    output.mediaPolicy,
                                                                    output.id)));
    }

    @Override
    public void exec(S3Endpoint endpoint, AmazonS3Client client, VerificationReporter reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");

        OrchestrationManagerEndpoint omEndpoint = endpoint.getOmEndpoint();

        omEndpoint.doVisit(_statVolumeOp);
    }

    private final StatVolume _statVolumeOp;
}
