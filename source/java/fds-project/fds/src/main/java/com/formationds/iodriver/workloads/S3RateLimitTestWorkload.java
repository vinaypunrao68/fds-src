package com.formationds.iodriver.workloads;

import java.time.Duration;
import java.time.ZonedDateTime;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.UUID;
import java.util.stream.Stream;

import com.codepoetics.protonpack.StreamUtils;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.operations.AddToReporter;
import com.formationds.iodriver.operations.CreateBucket;
import com.formationds.iodriver.operations.CreateObject;
import com.formationds.iodriver.operations.DeleteBucket;
import com.formationds.iodriver.operations.LambdaS3Operation;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.ReportStart;
import com.formationds.iodriver.operations.ReportStop;
import com.formationds.iodriver.operations.SetVolumeQos;
import com.formationds.iodriver.operations.StatVolume;
import com.formationds.iodriver.validators.RateLimitValidator;
import com.formationds.iodriver.validators.Validator;

/**
 * Workload that creates a single volume, sets its throttle at a given number of IOPS, and then
 * attempts to exceed those IOPS.
 */
public final class S3RateLimitTestWorkload extends Workload
{
    @Override
    public Optional<Validator> getSuggestedValidator()
    {
        return Optional.of(new RateLimitValidator());
    }
    
    /**
     * Constructor.
     * 
     * @param iops The number of IOPS to set throttle to.
     * @param logOperations Whether to log all operations executed by this workload.
     */
    public S3RateLimitTestWorkload(int iops, boolean logOperations)
    {
        super(logOperations);
        
        if (iops <= 0)
        {
            throw new IllegalArgumentException("Must have some IOPS to test. " + iops
                                               + " IOPS is not testable.");
        }

        _bucketName = UUID.randomUUID().toString();
        _iops = iops;
        _objectName = UUID.randomUUID().toString();
        _originalState = null;
        _stopTime = null;
        _targetState = null;
    }
    
    @Override
    public Class<?> getEndpointType()
    {
        return FdsEndpoint.class;
    }

    @Override
    protected List<Stream<Operation>> createOperations()
    {
        // Creates the same object 100 times with random content. Same thing actual load will do,
        // 100 times should be enough to get the right stuff in cache so we're running at steady
        // state.
        Stream<Operation> warmup =
                Stream.generate(() -> UUID.randomUUID().toString())
                      .<Operation>map(content -> new CreateObject(_bucketName,
                                                                    _objectName,
                                                                    content,
                                                                    false))
                      .limit(100);

        Stream<Operation> reportStart = Stream.of(new ReportStart(_bucketName));

        // Set the stop time for 10 seconds from when this operation is hit.
        Stream<Operation> startTestTiming = Stream.of(new LambdaS3Operation(() ->
        {
            if (_stopTime == null)
            {
                _stopTime = ZonedDateTime.now().plus(Duration.ofSeconds(10));
            }
        }));

        // The full test load, just creates the same object repeatedly with different content as
        // fast as possible for 10 seconds.
        Stream<Operation> load =
                StreamUtils.takeWhile(Stream.generate(() -> UUID.randomUUID().toString())
                                            .map(content -> new CreateObject(_bucketName,
                                                                             _objectName,
                                                                             content)),
                                      op -> ZonedDateTime.now().isBefore(_stopTime));

        // Report completion of the test.
        Stream<Operation> reportStop = Stream.of(new ReportStop(_bucketName));

        Stream<Operation> retval = Stream.empty();
        retval = Stream.concat(retval, warmup);
        retval = Stream.concat(retval, reportStart);
        retval = Stream.concat(retval, startTestTiming);
        retval = Stream.concat(retval, load);
        retval = Stream.concat(retval, reportStop);
        return Collections.singletonList(retval);
    }

    @Override
    protected Stream<Operation> createSetup()
    {
        return Stream.of(new CreateBucket(_bucketName),
                         new StatVolume(_bucketName, settings -> _originalState = settings),
                         new LambdaS3Operation(() ->
                         {
                             _targetState =
                                     new VolumeQosSettings(_originalState.getId(),
                                                           _originalState.getIopsAssured(),
                                                           _iops,
                                                           _originalState.getPriority(),
                                                           _originalState.getCommitLogRetention(),
                                                           _originalState.getMediaPolicy());
                         }),
                         new SetVolumeQos(() -> _targetState),
                         new AddToReporter(_bucketName, () -> _targetState));
    }

    @Override
    protected Stream<Operation> createTeardown()
    {
        return Stream.of(new DeleteBucket(_bucketName));
    }

    /**
     * The name of the bucket to use for testing.
     */
    private final String _bucketName;

    /**
     * The IOPS to set the volume throttle to.
     */
    private final int _iops;

    /**
     * The name of the object to repeatedly create.
     */
    private final String _objectName;

    /**
     * The QOS settings the volume was created with. {@code null} until
     * {@link #setUp(S3Endpoint, com.formationds.iodriver.reporters.WorkflowEventListener) setUp()}
     * has been run.
     */
    private VolumeQosSettings _originalState;

    /**
     * Stop running the main body of the workload at this time. {@code null} until the warmup of the
     * main test body is complete.
     */
    private ZonedDateTime _stopTime;

    /**
     * The QOS settings to set on the created volume. Same as {@link #_originalState}, but throttled
     * to {@link #_iops}.
     */
    private VolumeQosSettings _targetState;
}
