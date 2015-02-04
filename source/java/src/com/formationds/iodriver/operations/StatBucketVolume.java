package com.formationds.iodriver.operations;

import java.util.function.Consumer;

import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.WorkflowEventListener;

public final class StatBucketVolume extends S3Operation
{
    public StatBucketVolume(String bucketName, Consumer<VolumeQosSettings> consumer)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");
        if (consumer == null) throw new NullArgumentException("consumer");

        _statVolumeOp = new StatVolume(bucketName, consumer);
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

        omEndpoint.doVisit(_statVolumeOp, listener);
    }

    private final StatVolume _statVolumeOp;
}
