package com.formationds.iodriver.endpoints;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.operations.OmV7Operation;
import com.formationds.iodriver.operations.OmV8Operation;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.S3Operation;

public final class FdsEndpoint implements Endpoint
{
    public FdsEndpoint(OmV7Endpoint omV7Endpoint, OmV8Endpoint omV8Endpoint, S3Endpoint s3Endpoint)
    {
        if (omV7Endpoint == null) throw new NullArgumentException("omV7Endpoint");
        if (omV8Endpoint == null) throw new NullArgumentException("omV8Endpoint");
        if (s3Endpoint == null) throw new NullArgumentException("s3Endpoint");
        
        _omV7Endpoint = omV7Endpoint;
        _omV8Endpoint = omV8Endpoint;
        _s3Endpoint = s3Endpoint;
    }

    public FdsEndpoint copy()
    {
        _CopyHelper copyHelper = new _CopyHelper();
        
        return new FdsEndpoint(copyHelper);
    }

    public OmV7Endpoint getOmV7Endpoint()
    {
        return _omV7Endpoint;
    }
    
    public OmV8Endpoint getOmV8Endpoint()
    {
        return _omV8Endpoint;
    }
    
    public S3Endpoint getS3Endpoint()
    {
        return _s3Endpoint;
    }
    
    @Override
    public void visit(Operation operation,
                      WorkloadContext context) throws ExecutionException
    {
        if (operation == null) throw new NullArgumentException("operation");
        if (context == null) throw new NullArgumentException("context");
        
        if (operation instanceof OmV7Operation)
        {
            getOmV7Endpoint().visit((OmV7Operation)operation, context);
        }
        else if (operation instanceof OmV8Operation)
        {
            getOmV8Endpoint().visit((OmV8Operation)operation, context);
        }
        else if (operation instanceof S3Operation)
        {
            getS3Endpoint().visit((S3Operation)operation, context);
        }
        else
        {
            operation.accept(this, context);
        }
    }
    
    private final class _CopyHelper
    {
        public final OmV7Endpoint omV7Endpoint = _omV7Endpoint;
        public final OmV8Endpoint omV8Endpoint = _omV8Endpoint;
        public final S3Endpoint s3Endpoint = _s3Endpoint;
    }
    
    private FdsEndpoint(_CopyHelper copyHelper)
    {
        _omV7Endpoint = copyHelper.omV7Endpoint;
        _omV8Endpoint = copyHelper.omV8Endpoint;
        _s3Endpoint = copyHelper.s3Endpoint;
    }
    
    private final OmV7Endpoint _omV7Endpoint;
    
    private final OmV8Endpoint _omV8Endpoint;
    
    private final S3Endpoint _s3Endpoint;
}
