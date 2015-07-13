package com.formationds.iodriver.endpoints;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public final class FdsEndpoint implements Endpoint
{
    public FdsEndpoint(OmEndpoint omEndpoint, S3Endpoint s3Endpoint)
    {
        if (omEndpoint == null) throw new NullArgumentException("omEndpoint");
        if (s3Endpoint == null) throw new NullArgumentException("s3Endpoint");
        
        _omEndpoint = omEndpoint;
        _s3Endpoint = s3Endpoint;
    }

    public FdsEndpoint copy()
    {
        _CopyHelper copyHelper = new _CopyHelper();
        
        return new FdsEndpoint(copyHelper);
    }

    public OmEndpoint getOmEndpoint()
    {
        return _omEndpoint;
    }
    
    public S3Endpoint getS3Endpoint()
    {
        return _s3Endpoint;
    }

    @Override
    public void visit(Operation operation,
                      AbstractWorkflowEventListener listener) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (listener == null) throw new NullArgumentException("listener");
        
        operation.accept(this, listener);
    }
    
    private final class _CopyHelper
    {
        public final OmEndpoint omEndpoint = FdsEndpoint.this._omEndpoint;
        public final S3Endpoint s3Endpoint = FdsEndpoint.this._s3Endpoint;
    }
    
    private FdsEndpoint(_CopyHelper copyHelper)
    {
        _omEndpoint = copyHelper.omEndpoint;
        _s3Endpoint = copyHelper.s3Endpoint;
    }
    
    private final OmEndpoint _omEndpoint;
    
    private final S3Endpoint _s3Endpoint;
}
