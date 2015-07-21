package com.formationds.iodriver.workloads;

import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.operations.S3Operation;

public class S3Workload extends Workload<S3Endpoint, S3Operation>
{
    protected S3Workload(boolean logOperations)
    {
        super(S3Endpoint.class, S3Operation.class, logOperations);
    }
}
