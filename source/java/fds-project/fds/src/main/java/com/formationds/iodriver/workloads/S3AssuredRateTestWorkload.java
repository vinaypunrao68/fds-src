package com.formationds.iodriver.workloads;

import java.io.Closeable;
import java.io.PrintStream;
import java.time.Duration;
import java.time.ZonedDateTime;
import java.util.List;
import java.util.Optional;
import java.util.UUID;
import java.util.concurrent.CyclicBarrier;
import java.util.function.Consumer;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import com.codepoetics.protonpack.StreamUtils;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.operations.AwaitGate;
import com.formationds.iodriver.operations.CreateBucket;
import com.formationds.iodriver.operations.CreateObject;
import com.formationds.iodriver.operations.DeleteBucket;
import com.formationds.iodriver.operations.LambdaS3Operation;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.ReportStart;
import com.formationds.iodriver.operations.ReportStop;
import com.formationds.iodriver.operations.SetVolumeQos;
import com.formationds.iodriver.operations.StatVolume;
import com.formationds.iodriver.reporters.QosProgressReporter;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.validators.AssuredRateValidator;
import com.formationds.iodriver.validators.Validator;
import com.google.common.collect.ImmutableMap;

/**
 * Workload that ensures that a volume always receives its assured IOPS rate.
 */
public final class S3AssuredRateTestWorkload extends Workload
{
    @Override
    public Optional<Validator> getSuggestedValidator()
    {
        return Optional.of(new AssuredRateValidator());
    }
    
    @Override
    public Optional<Closeable> getSuggestedReporter(PrintStream output,
                                                    WorkloadEventListener listener)
    {
        if (output == null) throw new NullArgumentException("output");
        if (listener == null) throw new NullArgumentException("listener");
        
        return Optional.of(new QosProgressReporter(output, listener));
    }

    /**
     * Constructor.
     *
     * @param competingBuckets The number of buckets to compete against the test bucket's assured
     *            rate.
     * @param systemThrottle The maximum IOPS allowed by the system.
     */
    public S3AssuredRateTestWorkload(int competingBuckets,
                                     int systemThrottle)
    {
        if (competingBuckets < 1)
        {
            throw new IllegalArgumentException("competingBuckets must be at least 1, passed "
                                               + competingBuckets + ".");
        }
        if (systemThrottle < 1)
        {
            throw new IllegalArgumentException("systemThrottle cannot be less than 1, passed "
                                               + systemThrottle + ".");
        }
        if (systemThrottle - (competingBuckets + 1) * VOLUME_HARD_MIN < MEASURABLE_IO_THRESHOLD)
        {
            throw new IllegalArgumentException("With a hard minimum of " + VOLUME_HARD_MIN
                                               + " IOPS and " + competingBuckets
                                               + " competingBuckets, the system throttle of "
                                               + systemThrottle + " does not leave headroom of "
                                               + MEASURABLE_IO_THRESHOLD + " IOPS.");
        }

        final String assuredBucketName = UUID.randomUUID().toString();

        ImmutableMap.Builder<String, BucketState> bucketStatesBuilder = ImmutableMap.builder();
        for (int i = 0; i <= competingBuckets; ++i)
        {
            String bucketName = i == 0 ? assuredBucketName : UUID.randomUUID().toString();
            bucketStatesBuilder.put(bucketName, new BucketState());
        }

        _assuredBucketName = assuredBucketName;
        _bucketStates = bucketStatesBuilder.build();
        _competingBuckets = competingBuckets;
        _objectName = UUID.randomUUID().toString();
        _systemThrottle = systemThrottle;
        _warmupGate = new CyclicBarrier(competingBuckets + 1);
    }

    @Override
    public boolean doDryRun()
    {
        return true;
    }
    
    @Override
    public Class<?> getEndpointType()
    {
        return FdsEndpoint.class;
    }
    
    @Override
    protected List<Stream<Operation>> createOperations()
    {
        Stream<String> bucketNames =
                StreamSupport.stream(_bucketStates.keySet().spliterator(), false);

        Stream<Stream<Operation>> operations =
                bucketNames.map(bucketName -> createBucketOperations(bucketName));

        List<Stream<Operation>> retval = operations.collect(Collectors.toList());
        return retval;
    }

    @Override
    protected Stream<Operation> createSetup()
    {
        return Stream.concat(createSetupAssuredBucket(), createSetupCompetingBuckets());
    }

    @Override
    protected Stream<Operation> createTeardown()
    {
        return StreamSupport.stream(_bucketStates.keySet().spliterator(), false)
                            .map(bucketName -> new DeleteBucket(bucketName));
    }

    /**
     * State needed by this workload for each bucket.
     */
    private static final class BucketState
    {
        public VolumeQosSettings qosSettings;
        public ZonedDateTime stopTestTime;
    }

    /**
     * The name of the test bucket.
     */
    private final String _assuredBucketName;

    /**
     * State for all buckets.
     */
    private final ImmutableMap<String, BucketState> _bucketStates;

    /**
     * The number of buckets to create to compete with the test buckets.
     */
    private final int _competingBuckets;

    /**
     * The name of the object that's repeatedly created to generate I/Os.
     */
    private final String _objectName;

    /**
     * The maximum IOPS allowed by the system.
     */
    private final int _systemThrottle;

    /**
     * Wait for this gate to begin the actual workload.
     */
    private final CyclicBarrier _warmupGate;

    /**
     * The minimum number of IOPS that is a large enough difference that we're confident that the
     * system is throttling other volumes in order to fulfill another volume's minimum IOPS.
     */
    private final static int MEASURABLE_IO_THRESHOLD;

    /**
     * How long the test will run.
     */
    private final static Duration TEST_DURATION;

    /**
     * Absolute minimum assured IOPS the system will accept.
     */
    private final static int VOLUME_HARD_MIN;

    /**
     * Create the operations for an individual bucket.
     *
     * @param bucketName The bucket to create the operations for.
     *
     * @return A list of operations to execute against the given bucket.
     */
    private Stream<Operation> createBucketOperations(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        // Creates the same object 100 times with random content. Same thing actual load will do,
        // 100 times should be enough to get the right stuff in cache so we're running at steady
        // state.
        Stream<Operation> warmup =
                Stream.generate(() -> UUID.randomUUID().toString())
                      .<Operation>map(content -> new CreateObject(bucketName,
                                                                    _objectName,
                                                                    content))
                      .limit(100);

        // Wait for everyone to warm up.
        Stream<Operation> awaitWarmup = Stream.of(new AwaitGate(_warmupGate));

        // Start timing the test.
        Stream<Operation> reportStart = Stream.of(new ReportStart(bucketName));

        // Set the stop time for 10 seconds from when this operation is hit. We don't want to do a
        // map lookup every operation, so capture the state instead of looking it up each time.
        BucketState bucketState = _bucketStates.get(bucketName);
        Stream<Operation> startTestTiming = Stream.of(new LambdaS3Operation(() ->
        {
            if (bucketState.stopTestTime == null)
            {
                bucketState.stopTestTime = ZonedDateTime.now().plus(TEST_DURATION);
            }
        }));

        // The full test load, just creates the same object repeatedly with different content as
        // fast as possible for 10 seconds.
        Stream<Operation> load =
                StreamUtils.takeWhile(Stream.generate(() -> UUID.randomUUID().toString())
                                            .map(content -> new CreateObject(bucketName,
                                                                             _objectName,
                                                                             content)),
                                      op -> ZonedDateTime.now().isBefore(bucketState.stopTestTime));

        // Report completion of the test.
        Stream<Operation> reportStop = Stream.of(new ReportStop(bucketName));

        Stream<Operation> retval = Stream.empty();
        retval = Stream.concat(retval, warmup);
        retval = Stream.concat(retval, awaitWarmup);
        retval = Stream.concat(retval, reportStart);
        retval = Stream.concat(retval, startTestTiming);
        retval = Stream.concat(retval, load);
        retval = Stream.concat(retval, reportStop);
        return retval;
    }

    /**
     * Create the operations that will set up the assured (test) bucket.
     *
     * @return A list of operations.
     */
    private Stream<Operation> createSetupAssuredBucket()
    {
        int minAssuredIops = _competingBuckets * VOLUME_HARD_MIN;
        int headroom = _systemThrottle - minAssuredIops;
        int testAssured = (int)(headroom * 0.85);  // Take 85% of the system's I/O capacity to
                                                   // ensure there's really competition.

        return createSetupBucket(_assuredBucketName, testAssured);
    }

    /**
     * Create the operations that will set up a bucket that competes with the test bucket for IOs.
     *
     * @param bucketName The name of the bucket the operations will create.
     *
     * @return A list of operations.
     */
    private Stream<Operation> createSetupCompetingBucket(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        return createSetupBucket(bucketName, VOLUME_HARD_MIN);
    }

    /**
     * Create the operations that will set up all of the buckets that will compete with the test
     * bucket for IOs.
     *
     * @return A list of operations.
     */
    private Stream<Operation> createSetupCompetingBuckets()
    {
        return StreamSupport.stream(_bucketStates.keySet().spliterator(), false)
                            .filter(bucketName -> !bucketName.equals(_assuredBucketName))
                            .flatMap(bucketName -> createSetupCompetingBucket(bucketName));
    }

    /**
     * Create the operations that will set up a bucket.
     *
     * @param bucketName The name of the bucket the operations will create.
     * @param assured The assured IOPS for the created bucket.
     *
     * @return A list of operations.
     */
    private Stream<Operation> createSetupBucket(String bucketName, int assured)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        Supplier<VolumeQosSettings> qosGetter = () -> _bucketStates.get(bucketName).qosSettings;
        Consumer<VolumeQosSettings> qosSetter = qosSettings ->
        {
            BucketState state = _bucketStates.get(bucketName);
            state.qosSettings = qosSettings;
        };

        Runnable setQosParams = () ->
        {
            VolumeQosSettings current = qosGetter.get();
            VolumeQosSettings target =
                    new VolumeQosSettings(current.getId(),
                                          assured,
                                          0,
                                          current.getPriority(),
                                          current.getCommitLogRetention(),
                                          current.getMediaPolicy());
            qosSetter.accept(target);
        };

        CreateBucket createBucket = new CreateBucket(bucketName);
        StatVolume statBucket = new StatVolume(bucketName, qosSetter);
        LambdaS3Operation setTarget = new LambdaS3Operation(setQosParams);
        SetVolumeQos setQosToTarget = new SetVolumeQos(qosGetter);

        return Stream.of(createBucket, statBucket, setTarget, setQosToTarget);
    }

    static
    {
        // If the duration of the test is changed, measurable # of IOs should change as well.
        // TODO: Figure the relationship instead of guessing.
        MEASURABLE_IO_THRESHOLD = 50;
        TEST_DURATION = Duration.ofSeconds(10);
        VOLUME_HARD_MIN = 20;
    }
}
