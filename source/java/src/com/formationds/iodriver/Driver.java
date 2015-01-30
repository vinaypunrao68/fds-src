package com.formationds.iodriver;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.reporters.VerificationReporter;
import com.formationds.iodriver.workloads.Workload;

public final class Driver<EndpointT extends Endpoint<EndpointT, ?>,
                          WorkloadT extends Workload<EndpointT, ?>>
{
    public Driver(EndpointT endpoint, WorkloadT workload, VerificationReporter reporter)
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (workload == null) throw new NullArgumentException("workload");
        if (reporter == null) throw new NullArgumentException("reporter");
        
        _endpoint = endpoint;
        _reporter = reporter;
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
    
    public VerificationReporter getReporter()
    {
        return _reporter;
    }
    
    public int getResult()
    {
        if (getReporter().wereIopsInRange())
        {
            return 0;
        }
        else
        {
            return 1;
        }
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
    
    private final VerificationReporter _reporter;
    
    private final WorkloadT _workload;
}
