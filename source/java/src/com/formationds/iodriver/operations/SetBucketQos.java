package com.formationds.iodriver.operations;

import java.util.function.Supplier;

import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.WorkflowEventListener;

public final class SetBucketQos extends S3Operation
{
    public SetBucketQos(VolumeQosSettings input)
    {
        this(createStaticSupplier(input));
    }

    public SetBucketQos(Supplier<VolumeQosSettings> statsSupplier)
    {
        if (statsSupplier == null) throw new NullArgumentException("statsSupplier");

        _statsSupplier = statsSupplier;
    }

    @Override
    // @eclipseFormat:off
    public void exec(S3Endpoint endpoint,
                     AmazonS3Client client,
                     WorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (listener == null) throw new NullArgumentException("listener");

        OrchestrationManagerEndpoint omEndpoint = endpoint.getOmEndpoint();

        omEndpoint.doVisit(getSetVolumeQosOp(), listener);
    }

    private final Supplier<VolumeQosSettings> _statsSupplier;

    private SetVolumeQos getSetVolumeQosOp()
    {
        VolumeQosSettings stats = _statsSupplier.get();
        SetVolumeQos op = new SetVolumeQos(stats);

        return op;
    }

    private static Supplier<VolumeQosSettings> createStaticSupplier(VolumeQosSettings input)
    {
        if (input == null) throw new NullArgumentException("input");

        return () -> input;
    }
}
