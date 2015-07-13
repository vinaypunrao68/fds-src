package com.formationds.iodriver.endpoints;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.FdsOperation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public final class FdsEndpoint extends AbstractEndpoint<FdsEndpoint, FdsOperation>
{
    public FdsEndpoint(OrchestrationManagerEndpoint omEndpoint, S3Endpoint s3Endpoint)
    {
        super(FdsOperation.class);
        
        if (omEndpoint == null) throw new NullArgumentException("omEndpoint");
        if (s3Endpoint == null) throw new NullArgumentException("s3Endpoint");
        
        _omEndpoint = omEndpoint;
        _s3Endpoint = s3Endpoint;
    }

    @Override
    public FdsEndpoint copy()
    {
        _CopyHelper copyHelper = new _CopyHelper();
        
        return new FdsEndpoint(copyHelper);
    }

    @Override
    public void doVisit(FdsOperation operation,
                        AbstractWorkflowEventListener listener) throws ExecutionException
    {
        operation.accept(this, listener);
    }

    public OrchestrationManagerEndpoint getOmEndpoint()
    {
        return _omEndpoint;
    }
    
    public S3Endpoint getS3Endpoint()
    {
        return _s3Endpoint;
    }
    
    private final class _CopyHelper extends AbstractEndpoint<FdsEndpoint, FdsOperation>.CopyHelper
    {
        public final OrchestrationManagerEndpoint omEndpoint = FdsEndpoint.this._omEndpoint;
        public final S3Endpoint s3Endpoint = FdsEndpoint.this._s3Endpoint;
    }
    
    private FdsEndpoint(_CopyHelper copyHelper)
    {
        super(copyHelper);
        
        _omEndpoint = copyHelper.omEndpoint;
        _s3Endpoint = copyHelper.s3Endpoint;
    }
    
    private final OrchestrationManagerEndpoint _omEndpoint;
    
    private final S3Endpoint _s3Endpoint;
}
