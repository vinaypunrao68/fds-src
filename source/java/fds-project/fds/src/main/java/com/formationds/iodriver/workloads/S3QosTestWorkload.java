package com.formationds.iodriver.workloads;

import java.time.Duration;
import java.time.ZonedDateTime;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.function.Consumer;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import com.codepoetics.protonpack.StreamUtils;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.IoParams;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.operations.AddToReporter;
import com.formationds.iodriver.operations.CreateBucket;
import com.formationds.iodriver.operations.CreateObject;
import com.formationds.iodriver.operations.LambdaS3Operation;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.ReportStart;
import com.formationds.iodriver.operations.ReportStop;
import com.formationds.iodriver.operations.SetVolumeQos;
import com.formationds.iodriver.operations.StatVolume;

// TODO: This was intended to test the fairness QOS with multiple queues. Not complete.
public final class S3QosTestWorkload extends Workload
{
    public S3QosTestWorkload(Collection<IoParams> bucketParams,
                             Duration duration,
                             boolean logOperations)
    {
        this(bucketParams, duration, null, logOperations);
    }

    public S3QosTestWorkload(Collection<IoParams> bucketParams,
                             ZonedDateTime stopTime,
                             boolean logOperations)
    {
        this(bucketParams, null, stopTime, logOperations);
    }
    
    public boolean doDryRun()
    {
        return true;
    }
    
    @Override
    public Class<?> getEndpointType()
    {
        return S3Endpoint.class;
    }

    private S3QosTestWorkload(Collection<IoParams> bucketParams,
                              Duration duration,
                              ZonedDateTime stopTime,
                              boolean logOperations)
    {
        super(logOperations);
        
        if (bucketParams == null) throw new NullArgumentException("bucketParams");
        if ((duration == null) == (stopTime == null))
        {
            throw new IllegalArgumentException("Exactly one of duration or stopTime must be specified.");
        }

        _nameSupplier = () -> UUID.randomUUID().toString();

        _bucketStats =
                StreamSupport.stream(bucketParams.spliterator(), false)
                             .collect(Collectors.toMap(dummy -> _nameSupplier.get(),
                                                       value -> new BucketState(value)));
        _duration = duration;
        _objectName = _nameSupplier.get();
        _stopTime = stopTime;
    }

    protected Stream<Operation> createBucketOperations(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        Stream<Operation> warmup =
                Stream.generate(_nameSupplier)
                      .<Operation>map(content -> new CreateObject(bucketName,
                                                                  _objectName,
                                                                  content,
                                                                  false))
                      .limit(OPERATIONS_PER_BUCKET);

        Stream<Operation> unlimitedLoad =
                Stream.generate(_nameSupplier)
                      .<Operation>map(content -> new CreateObject(bucketName,
                                                                  _objectName,
                                                                  content));

        Stream<Operation> load =
                StreamUtils.takeWhile(unlimitedLoad, op -> ZonedDateTime.now().isBefore(_stopTime));

        Stream<Operation> retval = Stream.of(new ReportStart(bucketName));
        retval = Stream.concat(retval, warmup);
        retval = Stream.concat(retval, Stream.of(new LambdaS3Operation(() ->
        {
            if (_stopTime == null)
            {
                _stopTime = ZonedDateTime.now().plus(_duration);
            }
        })));
        retval = Stream.concat(retval, load);
        retval = Stream.concat(retval, Stream.of(new ReportStop(bucketName)));

        return retval;
    }

    protected Stream<Operation> CreateBucketSetup(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        Supplier<VolumeQosSettings> statsGetter =
                () -> _bucketStats.get(bucketName).currentState;
        Consumer<VolumeQosSettings> statsSetter = stats ->
        {
            BucketState state = _bucketStats.get(bucketName);
            state.currentState = stats;
        };

        return Stream.of(new CreateBucket(bucketName),
                         new StatVolume(bucketName, statsSetter),
                         new LambdaS3Operation(() ->
                         {
                             BucketState state = _bucketStats.get(bucketName);
                             state.currentState =
                                     new VolumeQosSettings(state.currentState.getId(),
                                                           state.targetState.getAssured(),
                                                           state.targetState.getThrottle(),
                                                           state.currentState.getPriority(),
                                                           state.currentState.getCommitLogRetention(),
                                                           state.currentState.getMediaPolicy());
                         }),
                         new SetVolumeQos(statsGetter),
                         new AddToReporter(bucketName, statsGetter));
    }

    @Override
    protected List<Stream<Operation>> createOperations()
    {
        return StreamSupport.stream(_bucketStats.keySet().spliterator(), false)
                            .map(bucketName -> createBucketOperations(bucketName))
                            .collect(Collectors.toList());
    }

    @Override
    protected Stream<Operation> createSetup()
    {
        return StreamSupport.stream(_bucketStats.keySet().spliterator(), false)
                            .flatMap(bucketName -> CreateBucketSetup(bucketName));
    }

    @Override
    protected Stream<Operation> createTeardown()
    {
        // return StreamSupport.stream(Lists.reverse(_bucketNames).spliterator(), false)
        // .map(bucketName -> new DeleteBucket(bucketName));
        // Stream<String> bucketNames =
        // StreamSupport.stream(Lists.reverse(_bucketNames).spliterator(), false);

        return Stream.empty();
    }

    private static class BucketState
    {
        public VolumeQosSettings currentState;
        public IoParams targetState;

        public BucketState(IoParams params)
        {
            targetState = params;
        }
    }

    private final Map<String, BucketState> _bucketStats;

    private final Duration _duration;

    private final Supplier<String> _nameSupplier;

    private final String _objectName;

    private ZonedDateTime _stopTime;

    private static final int OPERATIONS_PER_BUCKET;

    static
    {
        OPERATIONS_PER_BUCKET = 100;
    }
}
