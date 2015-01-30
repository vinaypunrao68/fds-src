package com.formationds.iodriver.operations;

import java.util.function.Supplier;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.apis.MediaPolicy;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.VerificationReporter;

public final class SetBucketQos extends S3Operation
{
    public static final class Input extends SetVolumeQos.Input
    {
        public Input(long id,
                     int assured_rate,
                     int throttle_rate,
                     int priority,
                     long commit_log_retention,
                     MediaPolicy mediaPolicy)
        {
            super(id, assured_rate, throttle_rate, priority, commit_log_retention, mediaPolicy);
        }
    }

    public SetBucketQos(Supplier<StatBucketVolume.Output> statsSupplier)
    {
        this(statsSupplier, null);
    }

    public SetBucketQos(Input input)
    {
        this(null, input);
    }

    @Override
    public void exec(S3Endpoint endpoint, AmazonS3Client client, VerificationReporter reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");

        OrchestrationManagerEndpoint omEndpoint = endpoint.getOmEndpoint();

        omEndpoint.doVisit(getSetVolumeQosOp());
    }

    private final Supplier<StatBucketVolume.Output> _statsSupplier;

    private SetVolumeQos _setVolumeQosOp;

    private SetBucketQos(Supplier<StatBucketVolume.Output> statsSupplier, Input input)
    {
        if ((statsSupplier == null) == (input == null))
        {
            throw new IllegalArgumentException("Exactly one of statsSupplier and input must be null.");
        }

        _setVolumeQosOp =
                input == null ? null
                        : new SetVolumeQos(new SetVolumeQos.Input(input.id,
                                                                  input.assured_rate,
                                                                  input.throttle_rate,
                                                                  input.priority,
                                                                  input.commit_log_retention,
                                                                  input.mediaPolicy));
        _statsSupplier = statsSupplier;
    }

    private SetVolumeQos getSetVolumeQosOp()
    {
        if (_setVolumeQosOp == null)
        {
            StatBucketVolume.Output stats = _statsSupplier.get();
            Input input = toInput(stats);
            _setVolumeQosOp = new SetVolumeQos(input);
        }
        return _setVolumeQosOp;
    }

    private static Input toInput(StatBucketVolume.Output bucketStats)
    {
        if (bucketStats == null) throw new NullArgumentException("bucketStats");

        return new Input(bucketStats.id,
                         bucketStats.assured_rate,
                         bucketStats.throttle_rate,
                         bucketStats.priority,
                         bucketStats.commit_log_retention,
                         bucketStats.mediaPolicy);
    }
}
