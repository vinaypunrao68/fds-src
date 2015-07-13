//package com.formationds.iodriver.operations;
//
//import java.util.AbstractMap.SimpleImmutableEntry;
//import java.util.function.Consumer;
//import java.util.stream.Stream;
//
//import com.amazonaws.services.s3.AmazonS3Client;
//import com.formationds.commons.NullArgumentException;
//import com.formationds.iodriver.endpoints.OmEndpoint;
//import com.formationds.iodriver.endpoints.S3Endpoint;
//import com.formationds.iodriver.model.VolumeQosSettings;
//import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
//
///**
// * Get the stats for the volume backing an S3 bucket.
// */
//public final class StatBucketVolume extends S3Operation
//{
//    /**
//     * Constructor.
//     * 
//     * @param bucketName The name of the bucket to stat.
//     * @param consumer Where to send the stats to.
//     */
//    public StatBucketVolume(String bucketName, Consumer<VolumeQosSettings> consumer)
//    {
//        if (bucketName == null) throw new NullArgumentException("bucketName");
//        if (consumer == null) throw new NullArgumentException("consumer");
//
//        _statVolumeOp = new StatVolume(bucketName, consumer);
//    }
//
//    @Override
//    // @eclipseFormat:off
//    public void accept(S3Endpoint endpoint,
//                       AbstractWorkflowEventListener listener) throws ExecutionException
//    // @eclipseFormat:on
//    {
//        if (endpoint == null) throw new NullArgumentException("endpoint");
//        if (listener == null) throw new NullArgumentException("listener");
//
//        OmEndpoint omEndpoint = endpoint.getOmEndpoint();
//
//        omEndpoint.visit(_statVolumeOp, listener);
//    }
//
//    @Override
//    public Stream<SimpleImmutableEntry<String, String>> toStringMembers()
//    {
//        return Stream.concat(super.toStringMembers(),
//                             Stream.of(memberToString("statVolumeOp", _statVolumeOp)));
//    }
//    
//    /**
//     * The OM operation to stat the volume backing the requested S3 bucket.
//     */
//    private final StatVolume _statVolumeOp;
//}
