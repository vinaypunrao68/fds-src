package com.formationds.iodriver.workloads;

import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.function.Supplier;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import com.formationds.iodriver.Workload;
import com.formationds.iodriver.operations.CreateBucket;
import com.formationds.iodriver.operations.CreateObject;
import com.formationds.iodriver.operations.DeleteBucket;
import com.formationds.iodriver.operations.Operation;
import com.google.common.collect.Lists;

public final class S3QosTestWorkload extends Workload
{
    public S3QosTestWorkload(long buckets)
    {
        if (buckets < 0) throw new IllegalArgumentException("buckets must be >= 0.");

        _nameSupplier = () -> UUID.randomUUID().toString();

        _bucketNames =
                Arrays.asList(Stream.generate(_nameSupplier)
                                    .limit(buckets)
                                    .toArray(size -> new String[size]));
        _objectName = _nameSupplier.get();
    }

    @Override
    protected Stream<Operation> createOperations()
    {
        Stream<String> bucketNames = StreamSupport.stream(_bucketNames.spliterator(), false);

        return bucketNames.<Operation>flatMap(bucketName ->
        {
            Stream<String> randomContent =
                    Stream.generate(_nameSupplier).limit(OPERATIONS_PER_BUCKET);

            return randomContent.map(content -> new CreateObject(bucketName,
                                                                 _objectName,
                                                                 content));
        });
    }

    @Override
    protected Stream<Operation> createSetup()
    {
        return StreamSupport.stream(_bucketNames.spliterator(), false)
                            .map(bucketName -> new CreateBucket(bucketName));
    }

    @Override
    protected Stream<Operation> createTeardown()
    {
        return StreamSupport.stream(Lists.reverse(_bucketNames).spliterator(), false)
                            .map(bucketName -> new DeleteBucket(bucketName));
    }

    private final List<String> _bucketNames;

    private final Supplier<String> _nameSupplier;

    private final String _objectName;

    private static final int OPERATIONS_PER_BUCKET;

    static
    {
        OPERATIONS_PER_BUCKET = 100;
    }
}
