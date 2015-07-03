package com.formationds.iodriver.operations;

import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.stream.Stream;

import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * Report workload body beginning.
 */
public class ReportStart extends S3Operation
{
    /**
     * Constructor.
     * 
     * @param bucketName The bucket to report begin for.
     */
    public ReportStart(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        _bucketName = bucketName;
    }

    @Override
    // @eclipseFormat:off
    public void exec(S3Endpoint endpoint,
                     AmazonS3Client client,
                     AbstractWorkflowEventListener reporter) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");

        reporter.reportStart(_bucketName);
    }

    @Override
    public Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.concat(super.toStringMembers(),
                             Stream.of(memberToString("bucketName", _bucketName)));
    }
    
    /**
     * The bucket to report begin for.
     */
    private final String _bucketName;
}
