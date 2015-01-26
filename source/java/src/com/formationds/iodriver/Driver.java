package com.formationds.iodriver;

import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.ExecutionException;

public final class Driver
{
    public Driver(Endpoint endpoint, Workload workload, Logger logger)
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (workload == null) throw new NullArgumentException("workload");
        if (logger == null) throw new NullArgumentException("logger");
        
        _endpoint = endpoint;
        _logger = logger;
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
    
    public Endpoint getEndpoint()
    {
        return _endpoint;
    }
    
    public Workload getWorkload()
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
    
    private Logger getLogger()
    {
        return _logger;
    }
    
    private final Endpoint _endpoint;
    
    private boolean _isSetUp;
    
    private final Logger _logger;
    
    private final Workload _workload;
}
