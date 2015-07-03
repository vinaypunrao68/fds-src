package com.formationds.iodriver.operations;

import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.function.Supplier;
import java.util.stream.Stream;

import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * Set the QoS parameters for the volume backing an S3 bucket.
 */
public final class SetBucketQos extends S3Operation
{
    /**
     * Constructor.
     * 
     * @param input The parameters to set.
     */
    public SetBucketQos(VolumeQosSettings input)
    {
        this(createStaticSupplier(input));
    }

    /**
     * Constructor.
     * 
     * @param statsSupplier Supplies parameters to set.
     */
    public SetBucketQos(Supplier<VolumeQosSettings> statsSupplier)
    {
        if (statsSupplier == null) throw new NullArgumentException("statsSupplier");

        _statsSupplier = statsSupplier;
    }

    @Override
    // @eclipseFormat:off
    public void exec(S3Endpoint endpoint,
                     AmazonS3Client client,
                     AbstractWorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (listener == null) throw new NullArgumentException("listener");

        OrchestrationManagerEndpoint omEndpoint = endpoint.getOmEndpoint();

        omEndpoint.getV8().doVisit(getSetVolumeQosOp(), listener);
    }

    @Override
    public Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.concat(super.toStringMembers(),
                             Stream.of(memberToString("statsSupplier", _statsSupplier)));
    }
    
    /**
     * Supplies parameters to set.
     */
    private final Supplier<VolumeQosSettings> _statsSupplier;

    /**
     * Create the underlying OM operation to set the QoS parameters on the bucket's volume.
     * 
     * @return QoS parameters.
     */
    private SetVolumeQos getSetVolumeQosOp()
    {
        VolumeQosSettings stats = _statsSupplier.get();
        SetVolumeQos op = new SetVolumeQos(stats);

        return op;
    }

    /**
     * Create a supplier that returns the same value repeatedly.
     * 
     * @param input The value to return.
     * 
     * @return {@code input} wrapped in a supplier.
     */
    private static Supplier<VolumeQosSettings> createStaticSupplier(VolumeQosSettings input)
    {
        if (input == null) throw new NullArgumentException("input");

        return () -> input;
    }
}
