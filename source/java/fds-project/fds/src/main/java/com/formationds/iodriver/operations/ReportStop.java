package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.WorkflowEventListener;

/**
 * Report workload body ending.
 */
public class ReportStop extends S3Operation
{
    /**
     * Constructor.
     * 
     * @param bucketName The bucket to report end for.
     */
    public ReportStop(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        _bucketName = bucketName;
    }

    @Override
    // @eclipseFormat:off
    public void exec(S3Endpoint endpoint,
                     AmazonS3Client client,
                     WorkflowEventListener reporter) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");

        reporter.reportStop(_bucketName);
    }

    /**
     * The bucket to report end for.
     */
    private final String _bucketName;
}
