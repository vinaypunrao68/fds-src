package com.formationds.iodriver.workloads;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.function.Consumer;
import java.util.function.Supplier;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.operations.AddToReporter;
import com.formationds.iodriver.operations.CreateBucket;
import com.formationds.iodriver.operations.CreateObject;
import com.formationds.iodriver.operations.ReportStart;
import com.formationds.iodriver.operations.ReportStop;
import com.formationds.iodriver.operations.S3Operation;
import com.formationds.iodriver.operations.SetBucketQos;
import com.formationds.iodriver.operations.StatBucketVolume;

public final class S3QosTestWorkload extends Workload<S3Endpoint, S3Operation>
{
    public S3QosTestWorkload(long buckets)
    {
        if (buckets < 0) throw new IllegalArgumentException("buckets must be >= 0.");

        _nameSupplier = () -> UUID.randomUUID().toString();

        _bucketNames =
                Arrays.asList(Stream.generate(_nameSupplier)
                                    .limit(buckets)
                                    .toArray(size -> new String[size]));
        _bucketStats = new HashMap<String, StatBucketVolume.Output>();
        _objectName = _nameSupplier.get();
    }

    @Override
    protected Stream<S3Operation> createOperations()
    {
        Stream<String> bucketNames = StreamSupport.stream(_bucketNames.spliterator(), false);

        return bucketNames.<S3Operation>flatMap(bucketName ->
        {
            Stream<String> randomContent =
                    Stream.generate(_nameSupplier).limit(OPERATIONS_PER_BUCKET);

            Stream<S3Operation> retval = Stream.of(new ReportStart(bucketName));
            retval = Stream.concat(retval, randomContent.map(content -> new CreateObject(bucketName,
                                                                                         _objectName,
                                                                                         content)));
            retval = Stream.concat(retval, Stream.of(new ReportStop(bucketName)));

            return retval;
        });
    }

    @Override
    protected Stream<S3Operation> createSetup()
    {
        return StreamSupport.stream(_bucketNames.spliterator(), false)
                            .flatMap(bucketName ->
                            {
                                Supplier<StatBucketVolume.Output> statsGetter =
                                        () -> _bucketStats.get(bucketName);
                                Consumer<StatBucketVolume.Output> statsSetter =
                                        stats -> _bucketStats.put(bucketName, stats);

                                return Stream.of(new CreateBucket(bucketName),
                                                 new StatBucketVolume(bucketName, statsSetter),
                                                 new SetBucketQos(statsGetter),
                                                 new AddToReporter(bucketName, statsGetter));
                            });
    }

    @Override
    protected Stream<S3Operation> createTeardown()
    {
        // return StreamSupport.stream(Lists.reverse(_bucketNames).spliterator(), false)
        // .map(bucketName -> new DeleteBucket(bucketName));
//        Stream<String> bucketNames =
//                StreamSupport.stream(Lists.reverse(_bucketNames).spliterator(), false);

        return Stream.empty();
    }

    private final List<String> _bucketNames;

    private final Map<String, StatBucketVolume.Output> _bucketStats;

    private final Supplier<String> _nameSupplier;

    private final String _objectName;

    private static final int OPERATIONS_PER_BUCKET;

    static
    {
        OPERATIONS_PER_BUCKET = 100;
    }
}
