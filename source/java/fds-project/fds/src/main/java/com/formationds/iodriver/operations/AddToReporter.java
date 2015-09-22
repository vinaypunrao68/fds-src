//package com.formationds.iodriver.operations;
//
//import java.util.AbstractMap.SimpleImmutableEntry;
//import java.util.function.Supplier;
//import java.util.stream.Stream;
//
//import com.formationds.commons.NullArgumentException;
//import com.formationds.iodriver.endpoints.Endpoint;
//import com.formationds.iodriver.model.VolumeQosSettings;
//import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;
//
///**
// * Add a given bucket to the reporter.
// */
//public class AddToReporter extends AbstractOperation
//{
//    /**
//     * Constructor.
//     * 
//     * @param bucketName The name of the bucket to add.
//     * @param statsGetter Gets the stats for {@code bucketName}.
//     */
//    public AddToReporter(String bucketName,
//                         Supplier<VolumeQosSettings> statsGetter)
//    {
//        if (bucketName == null) throw new NullArgumentException("bucketName");
//        if (statsGetter == null) throw new NullArgumentException("statsGetter");
//
//        _bucketName = bucketName;
//        _statsGetter = statsGetter;
//    }
//
//    @Override
//    public void accept(Endpoint endpoint,
//                       AbstractWorkloadEventListener reporter) throws ExecutionException
//    {
//        if (endpoint == null) throw new NullArgumentException("endpoint");
//        if (reporter == null) throw new NullArgumentException("reporter");
//
//        VolumeQosSettings stats = _statsGetter.get();
//        reporter.addVolume(_bucketName, stats);
//    }
//
//    @Override
//    protected Stream<SimpleImmutableEntry<String, String>> toStringMembers()
//    {
//        return Stream.concat(super.toStringMembers(),
//                             Stream.of(memberToString("bucketName", _bucketName),
//                                       memberToString("statsGetter", _statsGetter)));
//    }
//    
//    /**
//     * The name of the bucket to add.
//     */
//    private final String _bucketName;
//
//    /**
//     * Gets the stats for {@link #_bucketName}.
//     */
//    private final Supplier<VolumeQosSettings> _statsGetter;
//}
