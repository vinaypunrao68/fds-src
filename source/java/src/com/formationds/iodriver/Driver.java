package com.formationds.iodriver;

import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.operations.ExecutionException;

public final class Driver<EndpointT extends Endpoint<EndpointT, ?>,
                          WorkloadT extends Workload<EndpointT, ?>>
{
    public Driver(EndpointT endpoint, WorkloadT workload)
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (workload == null) throw new NullArgumentException("workload");
        
        _endpoint = endpoint;
        _workload = workload;
    }
    
    public void ensureSetUp() throws ExecutionException
    {
        if (!_isSetUp)
        {
            getWorkload().setUp(getEndpoint());
            _isSetUp = true;
        }
    }
    
    public void ensureTearDown() throws ExecutionException
    {
        if (_isSetUp)
        {
            getWorkload().tearDown(getEndpoint());
            _isSetUp = false;
        }
    }
    
    public EndpointT getEndpoint()
    {
        return _endpoint;
    }
    
    public WorkloadT getWorkload()
    {
        return _workload;
    }
    
    public void runWorkload() throws ExecutionException
    {
        ensureSetUp();
        try
        {
            getWorkload().runOn(getEndpoint());
        }
        finally
        {
            ensureTearDown();
        }
    }
    
    private final EndpointT _endpoint;
    
    private boolean _isSetUp;
    
    private final WorkloadT _workload;
}
